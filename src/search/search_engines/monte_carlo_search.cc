#include "monte_carlo_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../open_list_factory.h"
#include "../pruning_method.h"
#include "../task_utils/task_properties.h"

#include "../algorithms/ordered_set.h"
#include "../plugins/options.h"
#include "../task_utils/successor_generator.h"
#include "../utils/logging.h"
#include "../utils/rng_options.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional.hh>
#include <set>

using namespace std;

namespace mc_search {
MonteCarloSearch::MonteCarloSearch(const plugins::Options &opts)
    : SearchEngine(opts),
    rng(utils::parse_rng_from_options(opts)),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    explore_param(opts.get<double>("c"))
{}

void MonteCarloSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}

int MonteCarloSearch::selection() {

    int current_id = root_id;

    // selection should incorporate some bias using h
    while(monte_carlo_tree.at(current_id).is_fully_expanded) {
        current_id = monte_carlo_tree.at(current_id)
                        .max_uct_child(monte_carlo_tree, explore_param);
    }

    return current_id;

}

SearchStatus MonteCarloSearch::expansion(int selected_id) {

    int to_expand_id = selected_id;
    MCTS_node &expanded = monte_carlo_tree.at(to_expand_id);
    State parent_s = state_registry.lookup_state(monte_carlo_tree.at(to_expand_id).state);
    SearchNode node = search_space.get_node(parent_s);
    // node.close();    // nodes should only be cosed when pruned toward optimal
    // const State &s = node.get_state();
    
    if (check_goal_and_set_plan(parent_s))
        return SOLVED;

    // lazy evaluation of to_expand heuristic eval
    int parent_g = node.get_g();
    EvaluationContext parent_eval_context(
        parent_s, parent_g, false, &statistics);
    expanded.eval = parent_eval_context.get_evaluator_value(evaluator.get());

    // get successors of to_expand state
    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(parent_s, applicable_ops);

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((node.get_real_g() + op.get_cost()) >= bound)
            continue;

        State succ_state = state_registry.get_successor_state(parent_s, op);
        statistics.inc_generated();

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.
            
            succ_node.open(node, op, get_adjusted_cost(op));
            
            expanded.children_ids.push_back(next_id);
            monte_carlo_tree.emplace( next_id,
                MCTS_node(
                    next_id,
                    selected_id,
                    false,
                    0,
                    0,
                    0,
                    succ_state.get_id(),
                    op_id
                )
            );
            next_id+=1;
            
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state

            int mct_tree_id = mct_id[succ_node.get_state()];
            MCTS_node &mc_node = monte_carlo_tree.at(mct_tree_id);
            mc_node.parent_id = selected_id;

            succ_node.update_parent(node, op, get_adjusted_cost(op));
            
        }
    }

    if (expanded.next_child_i >= expanded.children_ids.size()-1)
        expanded.is_fully_expanded = true;

    return IN_PROGRESS;
}

RolloutScores MonteCarloSearch::rollout(int expanded_id, int curr_id) {

    MCTS_node &expanded = monte_carlo_tree.at(expanded_id);
    // cout << expanded.eval << endl;
    MCTS_node &curr = monte_carlo_tree.at(curr_id);
    OperatorID curr_op_id = curr.gen_op_id;
    State last_s = state_registry.lookup_state(expanded.state);
    OperatorProxy curr_op = task_proxy.get_operators()[curr_op_id];
    State curr_s = state_registry.lookup_state(curr.state);
    StateID last_id(curr_s.get_id());
    
    vector<MC_RolloutAction> rollout_stack = {};

    int g_budget = expanded.eval;
    int curr_eval;
    do {   

        SearchNode node = search_space.get_node(curr_s);
        int edge_cost = get_adjusted_cost(curr_op);
        // int curr_g = node.get_g() + edge_cost;
        EvaluationContext curr_eval_context(
            curr_s, 0, false, &statistics);
        rollout_stack.push_back(MC_RolloutAction(edge_cost, curr_op_id, curr_s.get_id()));

        if (task_properties::is_goal_state(task_proxy, curr_s)) {
            // this is gonna be anytime algo, so solving should just be a win
            vector<MC_RolloutAction>::iterator parent = rollout_stack.begin(); 
            vector<MC_RolloutAction>::iterator curr = rollout_stack.begin();

            for (curr = curr+1; curr != rollout_stack.end(); curr++) {
                State parent_s = state_registry.lookup_state(parent->gen_state_id);
                SearchNode p_node = search_space.get_node(parent_s);

                State path_s = state_registry.lookup_state(curr->gen_state_id);
                SearchNode c_node = search_space.get_node(path_s);
                OperatorProxy curr_op = task_proxy.get_operators()[curr->gen_op_id];
                c_node.open(p_node, curr_op, curr->edge_cost);
                parent = curr;

            }

            log << "Solution found!" << endl;
            Plan plan;
            search_space.trace_path(curr_s, plan);
            set_plan(plan);

            return SOLVE;
        }
        
        evaluator->notify_state_transition(last_s, curr_op_id, curr_s);
        curr_eval = curr_eval_context.get_evaluator_value_or_infinity(evaluator.get());

        if (curr_eval < expanded.eval) { // this is a design choice--define "progress" or "win"
            // return early -- set win, immediate success

            // potentail optimization: 
                // if a win happens, the found path should be added to real tree
                // so the search can "leapfrog" to points of progress.
            return WIN;
        }

        vector<OperatorID> applicable_ops;
        successor_generator.generate_applicable_ops(curr_s, applicable_ops);
        if (applicable_ops.empty()) {
            // deadend;
            return LOSE;
        }

        State keep_curr = curr_s; 
         // simple parent pruning. in future, use heuristic here to avoid 'invariants' (and pursue goals?)
        for(auto it = applicable_ops.begin(); it != applicable_ops.end(); it++) {
            curr_op_id = *it;
            curr_op = task_proxy.get_operators()[curr_op_id];
            curr_s = state_registry.get_successor_state(keep_curr, curr_op);
        }
        
        if (curr_s == last_s) {
             // also deadend--only path is back;
            return LOSE;
        }
        last_s = keep_curr;

        // only decrease this for brand new states
        //  never seen in a rollout either
        g_budget-=edge_cost;
    } while (g_budget > 0); // a large enough plateau would result in never winning

    // loss;
    return LOSE;   
}

void MonteCarloSearch::backpropogation(int to_back_prop_id, int score) {
    MCTS_node &to_back_prop = monte_carlo_tree.at(to_back_prop_id);

    int sim_count = 1; // if the score is bigger than 1 (the problem was solved), count it as two victories
    int outcome = score > 0 ? 1 : 0;
    to_back_prop.num_sims = sim_count;
    to_back_prop.num_wins = outcome;

    int parent_id = to_back_prop.parent_id;
    while (parent_id != -1) { // initial state's parent is -1
        MCTS_node &node = monte_carlo_tree.at(parent_id);
        node.num_sims+=sim_count;
        node.num_wins+=outcome;
        parent_id = node.parent_id;
    }
}

SearchStatus MonteCarloSearch::step() {
    
    int select_id = selection();
    if (expansion(select_id) == SOLVED) {
        return SOLVED;
    }

    MCTS_node &selected = monte_carlo_tree.at(select_id);
    int rollout_id = selected.children_ids[selected.next_child_i++];
    int score = rollout(select_id, rollout_id);
    if (score == SOLVE) {
        return SOLVED;
    }
    backpropogation(rollout_id, score);
    
    return IN_PROGRESS;
}

inline bool MonteCarloSearch::is_dead_end(
    EvaluationContext &eval_context) {
    return eval_context.is_evaluator_value_infinite(evaluator.get()) && 
        evaluator->dead_ends_are_reliable();
}

void MonteCarloSearch::initialize() {
    log << "Conducting Monte Carlo search with exploration parameter: " << explore_param << endl;

    State initial_state = state_registry.get_initial_state();
    evaluator->notify_initial_state(initial_state);

    /*
      Note: we consider the initial state as reached by a preferred
      operator.
    */
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (is_dead_end(eval_context)) {
        log << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            statistics.print_checkpoint_line(0);

        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();

        monte_carlo_tree.emplace(
            next_id,
            MCTS_node(
                next_id,
                -1,
                false,
                0,
                0,
                0,
                initial_state.get_id(),
                OperatorID::no_operator
            )
        );
    }
    root_id = next_id++;

    print_initial_evaluator_values(eval_context);

}

void MonteCarloSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void add_options_to_feature(plugins::Feature &feature) {
    SearchEngine::add_options_to_feature(feature);
    utils::add_rng_options(feature);
}
}


