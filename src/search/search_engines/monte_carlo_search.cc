#include "monte_carlo_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../open_list_factory.h"
#include "../pruning_method.h"

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
    auto &current = monte_carlo_tree.at(current_id);

    // selection should incorporate some bias using h
    while(!current.is_leaf) {
        current_id = current.max_uct_child(monte_carlo_tree, explore_param);
        current = monte_carlo_tree.at(current_id); // this should be an f(n) = uct(n) + wh(n) sort of thing where w is a parameter to weight heurisitc (careful to prioroitize LOWER h)
    }

    return current_id;

}

SearchStatus MonteCarloSearch::expansion(int selected_id) {

    int to_expand_id = selected_id;
    State parent_s = state_registry.lookup_state(monte_carlo_tree.at(to_expand_id).state);
    SearchNode node = search_space.get_node(parent_s);
    // const State &s = node.get_state();
    
    if (check_goal_and_set_plan(parent_s))
        return SOLVED;

    // lazy evaluation of to_expand heuristic eval
    int parent_g = node.get_g();
    EvaluationContext parent_eval_context(
        parent_s, parent_g, false, &statistics);
    monte_carlo_tree.at(selected_id).eval = parent_eval_context.get_evaluator_value(evaluator.get());

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
            
            monte_carlo_tree.at(selected_id).children_ids.push_back(next_id);
            monte_carlo_tree.emplace( next_id,
                MCTS_node(
                    next_id,
                    selected_id,
                    true,
                    0,
                    0,
                    succ_state.get_id(),
                    op_id
                )
            );
            next_id+=1;
            
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state.
            // if (reopen_closed_nodes) {
            //     if (succ_node.is_closed()) {
            //         /*
            //           TODO: It would be nice if we had a way to test
            //           that reopening is expected behaviour, i.e., exit
            //           with an error when this is something where
            //           reopening should not occur (e.g. A* with a
            //           consistent heuristic).
            //         */
            //         statistics.inc_reopened();
            //     }
            //     succ_node.reopen(*node, op, get_adjusted_cost(op));

            //     EvaluationContext succ_eval_context(
            //         succ_state, succ_node.get_g(), is_preferred, &statistics);

            //     /*
            //       Note: our old code used to retrieve the h value from
            //       the search node here. Our new code recomputes it as
            //       necessary, thus avoiding the incredible ugliness of
            //       the old "set_evaluator_value" approach, which also
            //       did not generalize properly to settings with more
            //       than one evaluator.

            //       Reopening should not happen all that frequently, so
            //       the performance impact of this is hopefully not that
            //       large. In the medium term, we want the evaluators to
            //       remember evaluator values for states themselves if
            //       desired by the user, so that such recomputations
            //       will just involve a look-up by the Evaluator object
            //       rather than a recomputation of the evaluator value
            //       from scratch.
            //     */
            //     open_list->insert(succ_eval_context, succ_state.get_id());
            // } else {
                // If we do not reopen closed nodes, we just update the parent pointers.
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back.
                succ_node.update_parent(node, op, get_adjusted_cost(op));
            // }
        }
    }

    monte_carlo_tree.at(selected_id).is_leaf = false;
    return IN_PROGRESS;
}

RolloutScores MonteCarloSearch::rollout(int expanded_id, int curr_id) {

    MCTS_node &expanded = monte_carlo_tree.at(expanded_id);
    MCTS_node &curr = monte_carlo_tree.at(curr_id);
    OperatorID curr_op_id = curr.gen_op_id;
    State last_s = state_registry.lookup_state(expanded.state);
    OperatorProxy curr_op = task_proxy.get_operators()[curr_op_id];
    State curr_s = state_registry.lookup_state(curr.state);

    int g_budget = expanded.eval;
    int last_eval = expanded.eval;
    int curr_eval;
    do {   

        if (check_goal_and_set_plan(curr_s)) {
            // this is gonna be anytime algo, so solving should just be a win
            return SOLVE;
        }

        SearchNode node = search_space.get_node(curr_s);
        int edge_cost = get_adjusted_cost(curr_op);
        int curr_g = node.get_g() + edge_cost;
        EvaluationContext curr_eval_context(
            curr_s, curr_g, false, &statistics);
        evaluator->notify_state_transition(last_s, curr_op_id, curr_s);
        curr_eval = curr_eval_context.get_evaluator_value(evaluator.get());

        if (curr_eval < last_eval) { // this is a design choice--define "progress" or "win"
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

        curr_op_id = *(rng->choose(applicable_ops));
        OperatorProxy op = task_proxy.get_operators()[curr_op_id];
        last_s = curr_s;
        curr_s = state_registry.get_successor_state(last_s, op);
        last_eval = curr_eval;

        // only decrease this for brand new states
        //  never seen in a rollout either
        g_budget-=edge_cost;
    } while (g_budget > 0); // a large enough plateau would result in never winning

    // loss;
    return LOSE;   
}

void MonteCarloSearch::backpropogation(int to_back_prop_id, int score) {
    MCTS_node &to_back_prop = monte_carlo_tree.at(to_back_prop_id);

    to_back_prop.is_leaf = true; 
    to_back_prop.num_sims = 1;
    to_back_prop.num_wins = score;

    int parent_id = to_back_prop.parent_id;
    while (parent_id != -1) { // initial state's parent is -1
        MCTS_node &node = monte_carlo_tree.at(parent_id);
        node.num_sims+=1;
        node.num_wins+=score;
        parent_id = node.parent_id;
    }
}

SearchStatus MonteCarloSearch::step() {
    

    int select_id = selection();
    if (expansion(select_id) == SOLVED) {
        return SOLVED;
    }

    MCTS_node &selected = monte_carlo_tree.at(select_id);
    int rollout_id = selected.children_ids[rng->random(selected.children_ids.size())];
    int outcome = rollout(select_id, rollout_id);
    backpropogation(rollout_id, outcome > 0 ? 1 : 0);
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
                true,
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


