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
#include "../task_proxy.h"

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

inline double MonteCarloSearch::uct(State &state) {
    MCTS_State &expanded = mc_tree_data[state];
    if (expanded.num_sims == 0) return std::numeric_limits<double>::max();

    double winrate = ((double) expanded.num_wins) / ((double) expanded.num_sims);
    return winrate + explore_param * sqrt(std::log((double) expanded.num_sims) 
        / ((double) expanded.num_sims));
};

inline StateID MonteCarloSearch::max_uct_child(vector<StateID> &children) {
    double max_uct = std::numeric_limits<double>::lowest();
    StateID max_id(StateID::no_state);
    for (StateID id: children) {
        State child = state_registry.lookup_state(id);
        double curr_uct = uct(child);
        if (curr_uct > max_uct) {
            max_id = id;
            max_uct = curr_uct;
        }
    }
    return max_id;
};

StateID MonteCarloSearch::selection() {

    StateID current_id = root_id;
    MCTS_State curr_state = mc_tree_data[state_registry.lookup_state(current_id)];

    // selection should incorporate some bias using h
    // backtracking required for deadends
    while(!curr_state.is_leaf) {
        current_id = max_uct_child(curr_state.children_ids); // fuck uct for now, just do epsilon greedy
        curr_state = mc_tree_data[state_registry.lookup_state(current_id)];
    }

    return current_id;

}

SearchStatus MonteCarloSearch::child_expansion(StateID to_expand_id) {

    tl::optional<SearchNode> expand_node;
    State s = state_registry.lookup_state(to_expand_id);
    expand_node.emplace(search_space.get_node(s));


    expand_node->close();    // nodes should only be c'osed when pruned toward optimal
    // const State &s = node.get_state();

    // lazy evaluation of to_expand heuristic eval
    const State &to_expand_s = expand_node->get_state();
    MCTS_State &expanded = mc_tree_data[to_expand_s];

    // get successors of to_expand state
    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(to_expand_s, applicable_ops);

    //see pruning in eager

    int count_wins = 0; 
    int total_children = 0;
    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((expand_node->get_real_g() + op.get_cost()) >= bound)
            continue;

        State succ_state = state_registry.get_successor_state(to_expand_s, op);
        statistics.inc_generated();

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.
            
            succ_node.open(*expand_node, op, get_adjusted_cost(op));
            if (check_goal_and_set_plan(succ_state))
                return SOLVED;
            
            int succ_g = succ_node.get_g() + get_adjusted_cost(op);
            EvaluationContext succ_eval_context(
                succ_state, succ_g, false, &statistics);
            statistics.inc_evaluated_states();

            int curr_eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());
            if (curr_eval < expanded.eval) {
                count_wins += 1;
            }
            total_children+=1;

            expanded.children_ids.push_back(succ_state.get_id());
            mc_tree_data[succ_state] = 
                  MCTS_State(
                    next_mc_id++,
                    true,
                    curr_eval,
                    0,
                    0
                );
            
        } else if (succ_node.get_g() > expand_node->get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state

            succ_node.update_parent(*expand_node, op, get_adjusted_cost(op));
            
        }
    }

    expanded.is_leaf = false;
    expanded.num_sims = total_children;
    expanded.num_wins = count_wins;

    return IN_PROGRESS;
}

// RolloutScores MonteCarloSearch::rollout(StateID expand_id) {

//     State last_s = state_registry.lookup_state(expand_id);
//     MCTS_State &expand_info = mc_tree_data[last_s];
//     SearchNode last_node = search_space.get_node(last_s); 

//     // OperatorID curr_op_id = curr.gen_op_id;
//     StateID next_id = expand_info.children_ids[expand_info.next_child_i++];
//     if (expand_info.next_child_i >= expand_info.children_ids.size())
//         expand_info.is_fully_expanded = true;
//     State next_s = state_registry.lookup_state(next_id);
//     SearchNode next_node = search_space.get_node(next_s); 

//     vector<MC_RolloutAction> rollout_stack = {};

//     OperatorID curr_op_id(OperatorID::no_operator);
    
//     int curr_eval;
//     int runout_depth = 20;
//     for (int i=0; i<runout_depth; i++) {

//         if (task_properties::is_goal_state(task_proxy, next_s)) {
//             // this is gonna be anytime algo, so solving should just be a win
//             vector<MC_RolloutAction>::iterator parent = rollout_stack.begin(); 
//             vector<MC_RolloutAction>::iterator curr = rollout_stack.begin();

//             for (curr = curr+1; curr != rollout_stack.end(); curr++) {
//                 State parent_s = state_registry.lookup_state(parent->gen_state_id);
//                 SearchNode p_node = search_space.get_node(parent_s);

//                 State path_s = state_registry.lookup_state(curr->gen_state_id);
//                 SearchNode c_node = search_space.get_node(path_s);
//                 OperatorProxy curr_op = task_proxy.get_operators()[curr->gen_op_id];
//                 c_node.open(p_node, curr_op, curr->edge_cost);
//                 parent = curr;

//             }

//             log << "Solution found!" << endl;
//             Plan plan;
//             search_space.trace_path(next_s, plan);
//             set_plan(plan);

//             return SOLVE;
//         }
        
//         EvaluationContext eval_context(next_s);

//         int curr_eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
//         if (curr_eval < expand_info.eval) { // this is a design choice--define "progress" or "win"
//             // return early -- set win, immediate success

//             // potentail optimization: 
//                 // if a win happens, the found path should be added to real tree
//                 // so the search can "leapfrog" to points of progress.
//             return WIN;
//         }

//         vector<OperatorID> applicable_ops;
//         successor_generator.generate_applicable_ops(next_s, applicable_ops);
//         if (applicable_ops.empty()) {
//             // deadend;
//             return LOSE;
//         }

//         int j=0;
//         State keep_curr = next_s;
//         int edge_cost;
//         do {
//             curr_op_id = applicable_ops[j];
//             OperatorProxy curr_op = task_proxy.get_operators()[curr_op_id];
//             next_s = state_registry.get_successor_state(keep_curr, curr_op);
//             edge_cost = get_adjusted_cost(curr_op);
//             j+=1;
//         } while (next_s == last_s && j < applicable_ops.size()); //parent pruning
        
//         if (next_s == last_s) {
//              // also deadend--only path is back;
//             return LOSE;
//         }
//         last_s = keep_curr;


//         rollout_stack.push_back(MC_RolloutAction(edge_cost, curr_op_id, next_s.get_id()));
//     }

//     // loss;
//     return LOSE;   
// }

void MonteCarloSearch::backpropogation(StateID to_back_prop_id) {
    
    State back_s = state_registry.lookup_state(to_back_prop_id);
    MCTS_State &back_prop = mc_tree_data[back_s];
    SearchNode back_node = search_space.get_node(back_s);
    int plays = back_prop.num_sims;
    int wins = back_prop.num_wins;

    if (plays == 0)
        return;

    to_back_prop_id = back_node.get_info().parent_state_id;
    while (to_back_prop_id != StateID::no_state) {
        State last_s = state_registry.lookup_state(to_back_prop_id);
        MCTS_State &expand_info = mc_tree_data[last_s];
        expand_info.num_sims+=plays;
        expand_info.num_wins+=wins;

        SearchNode last_node = search_space.get_node(last_s); 
        to_back_prop_id = last_node.get_info().parent_state_id;
    }
}

SearchStatus MonteCarloSearch::step() {
    
    StateID select_id = selection();
    if (child_expansion(select_id) == SOLVED) {
        return SOLVED;
    }

    backpropogation(select_id);
    
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
        EvaluationContext eval_context(initial_state, 0, true, &statistics);
        int curr_eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());

        mc_tree_data[initial_state] =  MCTS_State(
                next_mc_id++,
                true,
                curr_eval,
                0,
                0
            );
    }
    root_id = initial_state.get_id();

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


