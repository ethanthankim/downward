#include "new_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../open_list_factory.h"
#include "../pruning_method.h"

#include "../algorithms/ordered_set.h"
#include "../plugins/options.h"
#include "../task_utils/successor_generator.h"
#include "../utils/rng_options.h"
#include "../utils/logging.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional.hh>
#include <set>
#include <random>



#include <chrono>

using namespace std;

namespace new_search {
NewSearch::NewSearch(const plugins::Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      tau(opts.get<double>("tau")),
      current_sum(0.0) {}

void NewSearch::initialize() {
    log << "Conducting new search"
        << (reopen_closed_nodes ? " with" : " without")
        << " reopening closed nodes, (real) bound = " << bound
        << endl;

    State initial_state = state_registry.get_initial_state();

    EvaluationContext eval_context(initial_state, 0, false, &statistics);
    int init_eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (init_eval != numeric_limits<int>::max()) {
        statistics.inc_evaluated_states();

        if (search_progress.check_progress(eval_context))
            statistics.print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();
        initial_expansion = true;

    }

    print_initial_evaluator_values(eval_context);

    // pruning_method->initialize(task);
}

void NewSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
    // pruning_method->print_statistics();
}

OperatorID NewSearch::get_next_rollout_start(State s) {

    int next_op_id = open_states[s].op_index++;
    if (next_op_id == open_states[s].operators.size() - 1) { // every successor has been rolled out from
        SearchNode node = search_space.get_node(s);
        node.close();
    }
    return open_states[s].operators[next_op_id];
    
}

OperatorID NewSearch::random_next_action(State s) {
    return *rng->choose(open_states[s].operators);
}

bool NewSearch::open_rollout_node(State s, OperatorID op_id, State parent_s) {
    SearchNode to_open = search_space.get_node(s);
    SearchNode parent = search_space.get_node(parent_s);
    OperatorProxy op = task_proxy.get_operators()[op_id];
    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);
    int num_ops = applicable_ops.size();
    if (num_ops == 0) {
        return false;
    }
    int cost = get_adjusted_cost(op);
    active_rollout_path_segments.back().push_back(s);

    if (to_open.is_new()) {

        rng->shuffle(applicable_ops); // randomize successor order
        open_states[s] = OpenState(open_states[parent_s].depth + 1, 0, false, applicable_ops);
        to_open.open(parent, op, cost);

        return true;
    } else if (to_open.get_g() > parent.get_g() + cost) { // open or closed, update g-cost and depth
        to_open.update_parent(parent, op, cost);
        open_states[s].depth = open_states[parent_s].depth + 1;
    }
    return true;
}

// do a greedy rollout until you hit a goal or the bottom of a crater.
// upon crater detection, will call random_rollout and will recurse if random_rollout finds HI
// return true if goal, false otherwise
NewSearch::RolloutResult NewSearch::greedy_rollout(const State rollout_state) {
    
    SearchNode rollout_node = search_space.get_node(rollout_state);
    int rollout_g = rollout_node.get_g();
    State curr_state = rollout_state;
    State next_state = curr_state;

    State parent_s = active_rollout_path_segments.back().back();
    active_rollout_path_segments.push_back({});
    if ( !open_rollout_node(rollout_state, rollout_node.get_info().creating_operator, parent_s)) {
        return UHR;
    }
    if (open_states[rollout_state].greedily_expanded == true){ // don't expand an already greedily expanded path...
        return UHR; // we're running toward a known crater
    }
    open_states[rollout_state].greedily_expanded = true;

    if (task_properties::is_goal_state(task_proxy, rollout_state))
        return GOAL;

    int path_len = 0;
    int curr_h = std::numeric_limits<int>::max();
    while (true) { // keep expanding while heuristic improvements come in

        vector<OperatorID> applicable_ops;
        successor_generator.generate_applicable_ops(curr_state, applicable_ops); 

        if (applicable_ops.size() == 0) {
            return UHR; // failure end of recursion: deadend
        }

        EvaluationContext eval_context(curr_state, &statistics, false);
        curr_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());

        int min_child_h = std::numeric_limits<int>::max();
        OperatorID chosen_op_id = OperatorID::no_operator;

        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            // if ((node->get_real_g() + op.get_cost()) >= bound)
            //     continue;

            State succ_state = state_registry.get_successor_state(curr_state, op);
            statistics.inc_generated();

            if (task_properties::is_goal_state(task_proxy, succ_state)) {
                open_rollout_node(succ_state, op_id, curr_state);
                return GOAL; // end of recursion: success
            }

            EvaluationContext succ_eval_context(
                    succ_state, &statistics, false);

            int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());


            if (eval < min_child_h) {
                chosen_op_id = op_id;
                min_child_h = eval;
                next_state = succ_state;
            }

        }

        if (min_child_h >= curr_h) {
            break;
        }

        open_rollout_node(next_state, chosen_op_id, curr_state); // ignored return value. maybe void?
        if (open_states[next_state].greedily_expanded == true){ // don't expand an already greedily expanded path...
            return UHR; // we're running toward a known crater
        }
        open_states[next_state].greedily_expanded = true;
        
        curr_state = next_state;
        path_len+=1;
    }
    
    return random_rollout(curr_state, curr_h, path_len);
     
}

// do a random rollout starting at rollout_node. assume rollout state already open
//return: true if HI, false otherwise
NewSearch::RolloutResult NewSearch::random_rollout(const State rollout_state, int start_h, int rollout_limit) {

    // do rollout
    OperatorID op_id = get_next_rollout_start(rollout_state);
    OperatorProxy op = task_proxy.get_operators()[op_id];
    State curr_state = state_registry.get_successor_state(rollout_state, op);
    State parent_s = rollout_state;
    while (rollout_limit > 0) {

        // open node
        if ( !open_rollout_node(curr_state, op_id, parent_s)) {
            return UHR;
        }

        if (task_properties::is_goal_state(task_proxy, curr_state)) {
            return GOAL; // end of recursion: success
        }

        op_id = random_next_action(curr_state);
        op = task_proxy.get_operators()[op_id];
        parent_s = curr_state;
        curr_state = state_registry.get_successor_state(curr_state, op);

        rollout_limit-=1;
    }

    SearchNode curr_node = search_space.get_node(curr_state);
    EvaluationContext succ_eval_context(
            curr_state, curr_node.get_g(), false, &statistics);
    int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());
    
    if (eval < start_h) { // if the rollout found an improvement
        return greedy_rollout(curr_state);
    } else {
        open_rollout_node(curr_state, op_id, parent_s); // otherwise add to failed path segment and return
        return UHR; // end of rollout recursion: failure mode 
    }
}

// pick just one child of expand_state and calculate h and include it in type system
/*
bool NewSearch::partially_expand_state(SearchNode& expand_node) {
    active_rollout_path_segments.clear();

    // get next operator from expand_state
    State expand_state = expand_node.get_state();
    OpenState info = open_states[expand_state];
    OperatorID op_id = info.operators[info.op_index++]; // get the next operator
    OperatorProxy op = task_proxy.get_operators()[op_id];

    // generate the successor and check if its a deadend
    State succ_state = state_registry.get_successor_state(expand_state, op);
    statistics.inc_generated();
    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(succ_state, applicable_ops);
    if (applicable_ops.size() == 0) { // deadend
        return false;
    } else {
        active_rollout_path_segments[0].push_back(succ_state); // if not a deadend, it's start of next path to be added to open
    }

    // calcualte h of the successor
    int succ_g = expand_node.get_g() + get_adjusted_cost(op);
    EvaluationContext succ_eval_context(
        succ_state, succ_g, false, &statistics);
    statistics.inc_evaluated_states();
    int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());

    if (info.op_index == info.operators.size()) {
        statistics.inc_expanded(); // only count fully expanded nodes (a little cheaty?)
        expand_node.close();
    }

    // compare heuristic to parent for HI and depth
    EvaluationContext p_eval_context(expand_state, &statistics, false);
    int parent_h = p_eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (eval < parent_h) {
        open_states[expand_state] = OpenState(0, 0, false, applicable_ops);
        return true;
    } else {
        open_states[expand_state] = OpenState(info.depth+1, 0, false, applicable_ops);
        return false;
    }

}
*/

SearchStatus NewSearch::step() {
    
    int selected_depth = 0;
    RolloutResult rollout_result; 
    tl::optional<SearchNode> node;
    while (true) {
        if (hi_types.empty()) {
            log << "Completely explored state space -- no solution!" << endl;
            return FAILED;
        }

        selected_depth = hi_types.begin()->first;
        if (hi_types.size() > 1) {
            double r = rng->random();
            
            double total_sum = current_sum;
            double p_sum = 0.0;
            for (auto it : hi_types) {
                double p = 1.0 / total_sum;
                p *= std::exp(-1.0*static_cast<double>(it.first) / tau); //remove -1.0 *
                p *= static_cast<double>(it.second.size());
                p_sum += p;
                if (r <= p_sum) {
                    selected_depth = it.first;
                    break;
                }
            }
        }

        vector<vector<StateID>> &types = hi_types.at(selected_depth);
        int chosen_type_i = rng->random(types.size());
        vector<StateID> &states = types[chosen_type_i];
        int chosen_i = rng->random(states.size());
        StateID id = states[chosen_i];
        State s = state_registry.lookup_state(id);
        node.emplace(search_space.get_node(s));

        if (node->is_closed()) { // lazy type system node removal
            utils::swap_and_pop_from_vector(states, chosen_i);
            if (states.empty()){
                utils::swap_and_pop_from_vector(types, chosen_type_i);
                if (types.empty()) {
                    hi_types.erase(selected_depth);
                    current_sum -= std::exp(-1.0*static_cast<double>(selected_depth) / tau);
                }
            }
            continue;
        }

        break;
    }


    State expanding_state = node->get_state();
    if (initial_expansion) {
        initial_expansion = false;
        vector<OperatorID> applicable_ops;
        successor_generator.generate_applicable_ops(expanding_state, applicable_ops); 
        int min_child_h = std::numeric_limits<int>::max();
        OperatorID chosen_op_id = OperatorID::no_operator;
        State next_state = expanding_state;
        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            // if ((node->get_real_g() + op.get_cost()) >= bound)
            //     continue;

            State succ_state = state_registry.get_successor_state(expanding_state, op);
            statistics.inc_generated();

            if (task_properties::is_goal_state(task_proxy, succ_state)) {
                open_rollout_node(succ_state, op_id, expanding_state);
                Plan plan;
                search_space.trace_path(active_rollout_path_segments.back().back(), plan);
                set_plan(plan);
                return SOLVED; // end of recursion: success
            }

            EvaluationContext succ_eval_context(
                    succ_state, &statistics, false);

            int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());


            if (eval < min_child_h) {
                chosen_op_id = op_id;
                min_child_h = eval;
                next_state = succ_state;
            }
        }

        active_rollout_path_segments.push_back({expanding_state});
        rollout_result = greedy_rollout(next_state);
    } else {
        int rollout_limit = open_states[node->get_state()].depth;
        OperatorProxy op = task_proxy.get_operators()[node->get_info().creating_operator];
        int succ_g = node->get_g() + get_adjusted_cost(op);
        EvaluationContext succ_eval_context(
            expanding_state, succ_g, false, &statistics);
        int h = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());
        rollout_result = random_rollout(expanding_state, h, rollout_limit);
    }

    if (rollout_result == GOAL) {
        Plan plan;
        search_space.trace_path(active_rollout_path_segments.back().back(), plan);
        set_plan(plan);
        return SOLVED;
    } else {

        // add node that results from partial expansion here
        int depth = selected_depth; // first path segment is un-evaled part of random_rollout and so always goes in parent's type
        // add the rolled out path to the type system
        for (auto path_segment : active_rollout_path_segments) {
            
            current_sum += std::exp(static_cast<double>(depth) / tau);
            std::vector<StateID> ids; 
            for (auto state : path_segment) {
        
                ids.push_back(state.get_id());
                
            }

            auto &types_at_depth = hi_types[depth];
            types_at_depth.push_back(ids);
            depth+=1;
        }

        return IN_PROGRESS;
    }
}

void NewSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    // open_list->boost_preferred();
}

void NewSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void NewSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    // if (f_evaluator) {
    //     int f_value = eval_context.get_evaluator_value(f_evaluator.get());
    //     statistics.report_f_value_progress(f_value);
    // }
}

/* TODO: HACK! This is very inefficient for simply looking up an h value.
   Also, if h values are not saved it would recompute h for each and every state. */
void NewSearch::update_f_value_statistics(EvaluationContext &eval_context) {
    // if (f_evaluator) {
    //     int f_value = eval_context.get_evaluator_value(f_evaluator.get());
    //     statistics.report_f_value_progress(f_value);
    // }
}

void add_options_to_feature(plugins::Feature &feature) {
    // SearchEngine::add_pruning_option(feature);
    SearchEngine::add_options_to_feature(feature);
}
}
