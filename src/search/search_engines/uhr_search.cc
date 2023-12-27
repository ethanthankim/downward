#include "uhr_search.h"

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



#include <chrono>

using namespace std;

namespace uhr_search {
UHRSearch::UHRSearch(const plugins::Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      tau(opts.get<double>("tau")),
      r_limit(opts.get<int>("r_limit")),
      current_sum(0.0) {}

void UHRSearch::initialize() {
    log << "Conducting new search"
        << (reopen_closed_nodes ? " with" : " without")
        << " reopening closed nodes, (real) bound = " << bound
        << endl;
    // assert(open_list);

    // set<Evaluator *> evals;


    /*
      Collect path-dependent evaluators that are used for preferred operators
      (in case they are not also used in the open list).
    */
    // for (const shared_ptr<Evaluator> &evaluator : preferred_operator_evaluators) {
    //     evaluator->get_path_dependent_evaluators(evals);
    // }

    /*
      Collect path-dependent evaluators that are used in the f_evaluator.
      They are usually also used in the open list and will hence already be
      included, but we want to be sure.
    */
    // if (f_evaluator) {
    //     f_evaluator->get_path_dependent_evaluators(evals);
    // }

    /*
      Collect path-dependent evaluators that are used in the lazy_evaluator
      (in case they are not already included).
    */
    // if (lazy_evaluator) {
    //     lazy_evaluator->get_path_dependent_evaluators(evals);
    // }

    /*
    Collect openlist path dependent evaluators
    */
    // open_list->get_path_dependent_evaluators(evals);

    // path_dependent_evaluators.assign(evals.begin(), evals.end());

    State initial_state = state_registry.get_initial_state();
    // for (Evaluator *evaluator : path_dependent_evaluators) {
    //     evaluator->notify_initial_state(initial_state);
    // }

    /*
      Note: we consider the initial state as reached by a preferred
      operator.
    */
    EvaluationContext eval_context(initial_state, 0, true, &statistics);
    int init_eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (init_eval != numeric_limits<int>::max()) {
        budget = 2*init_eval;
        statistics.inc_evaluated_states();

        if (search_progress.check_progress(eval_context))
            statistics.print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();

        types[init_eval].push_back(initial_state.get_id());
    }

    print_initial_evaluator_values(eval_context);

    // pruning_method->initialize(task);
}

void UHRSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
    // pruning_method->print_statistics();
}

OperatorID UHRSearch::random_next_action(State s) {
    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);
    int num_ops = applicable_ops.size();

    int j = 0;
    State next_s = s;
    int edge_cost;
    OperatorID curr_op_id = OperatorID::no_operator;
    do {
        curr_op_id = applicable_ops[j];
        OperatorProxy curr_op = task_proxy.get_operators()[curr_op_id];
        next_s = state_registry.get_successor_state(s, curr_op);
        j = (j + 1);
    } while (next_s == s && j < applicable_ops.size()); //parent pruning

    if (next_s == s) {
        // also deadend--only path is back;
        return OperatorID::no_operator;
    }

    return curr_op_id;
}

SearchStatus UHRSearch::greedy_rollout(State rollout_state) {

    SearchNode rollout_node = search_space.get_node(rollout_state);
    OperatorProxy roll_op = task_proxy.get_operators()[rollout_node.get_info().creating_operator];
    int succ_g = rollout_node.get_g() + get_adjusted_cost(roll_op);
    EvaluationContext rollout_eval_ctx(
        rollout_state, succ_g, false, &statistics);
    int rollout_eval = rollout_eval_ctx.get_evaluator_value_or_infinity(evaluator.get());
    int last_eval = rollout_eval;
    State last_state = rollout_state;
    int rollout_depth = state_depth[rollout_state];

    OperatorID next_op = random_next_action(rollout_state);
    if (next_op == OperatorID::no_operator) {
        return IN_PROGRESS;
    }
    OperatorProxy op = task_proxy.get_operators()[next_op];
    State curr_state = state_registry.get_successor_state(rollout_state, op);
    SearchNode curr_node = search_space.get_node(curr_state);

    if (curr_node.is_new())
        curr_node.open(rollout_node, op, get_adjusted_cost(op));
    
    statistics.inc_generated();

    int rollout_budget = budget; // idk
    bool found_hi = false;
    do {
        if (check_goal_and_set_plan(curr_state)) {
            return SOLVED;
        }

        EvaluationContext curr_eval_ctx(
            curr_state, succ_g, false, &statistics);
        int curr_eval = curr_eval_ctx.get_evaluator_value_or_infinity(evaluator.get());

        bool keep_open = false;
        if (curr_eval < last_eval && !found_hi) {

            state_depth[curr_state] = rollout_eval;
            types[rollout_eval].push_back(curr_state.get_id());

            current_sum += std::exp(-1.0*static_cast<double>(rollout_eval) / tau);

            // open_list->notify_state_transition(last_state, next_op, curr_state);
            // open_list->insert(curr_eval_ctx, curr_state.get_id());
            keep_open == true;
            found_hi = true;
        } 
        if (curr_eval < rollout_eval) {

            if (!keep_open) {
                state_depth[curr_state] = curr_eval;//rollout_depth+1;
                types[state_depth[curr_state]].push_back(curr_state.get_id());

                current_sum += std::exp(-1.0*static_cast<double>(state_depth[curr_state]) / tau);
            }
            // open_list->notify_state_transition(last_state, next_op, curr_state);
            // open_list->insert(curr_eval_ctx, curr_state.get_id());
            rollout_budget = budget;
            // return iterated_rollout(curr_state, limit);
        }

        next_op = random_next_action(rollout_state);
        if (next_op == OperatorID::no_operator) {
            return IN_PROGRESS;
        }
        op = task_proxy.get_operators()[next_op];
        State succ_state = state_registry.get_successor_state(curr_state, op);
        statistics.inc_generated();

        if(search_space.get_node(curr_state).is_open() && !keep_open)
            search_space.get_node(curr_state).close();

        SearchNode succ_node = search_space.get_node(succ_state);
        if (succ_node.is_new())
            succ_node.open(curr_node, op, get_adjusted_cost(op));
        last_state = curr_state;
        last_eval = curr_eval;
        curr_state = succ_state;

        if (search_progress.check_progress(curr_eval_ctx)) {
            statistics.print_checkpoint_line(succ_node.get_g());
            // reward_progress();
        }

    rollout_budget-=1;
    } while(rollout_budget > 0);

    return  IN_PROGRESS;

}

inline int UHRSearch::greedy_policy(vector<EvaluationContext>& succ_eval) {

    int best_i = -1;
    int best_eval = numeric_limits<int>::max();

    for (int i=0; i<succ_eval.size(); i++) {
        statistics.inc_evaluated_states();
        int eval = succ_eval[i].get_evaluator_value_or_infinity(evaluator.get());

        if (eval < best_eval){
            best_i = i;
            best_eval = eval;
        }
    }

    return best_i;
    
}

SearchStatus UHRSearch::random_rollout(State rollout_state, State parent_state) {
    
    SearchNode rollout_node = search_space.get_node(rollout_state);
    OperatorProxy roll_op = task_proxy.get_operators()[rollout_node.get_info().creating_operator];
    int succ_g = rollout_node.get_g() + get_adjusted_cost(roll_op);

    State last_state = parent_state;
    State curr_state = rollout_state;
    int type = state_to_type[rollout_state];
    int rollout_len = type_roll_len[type];

    vector<RolloutState> rollout_path;
    rollout_path.reserve(rollout_len);
    vector<int> g_costs;
    g_costs.reserve(rollout_len);

    while (rollout_len > 0) {

        rollout_path.push_back(RolloutState(curr_state, succ_g, );
        g_costs.push_back(succ_g);

        State tmp = curr_state;
        curr_state = random_policy(curr_state, last_state);
        last_state = tmp;

        rollout_len-=1;
    }

    OperatorID next_op = random_next_action(rollout_state);
    if (next_op == OperatorID::no_operator) {
        return IN_PROGRESS;
    }
    OperatorProxy op = task_proxy.get_operators()[next_op];
    State curr_state = state_registry.get_successor_state(rollout_state, op);
    SearchNode curr_node = search_space.get_node(curr_state);

    if (curr_node.is_new())
        curr_node.open(rollout_node, op, get_adjusted_cost(op));
    
    statistics.inc_generated();

    int rollout_budget = budget; // idk
    bool found_hi = false;
    do {
        if (check_goal_and_set_plan(curr_state)) {
            return SOLVED;
        }

        EvaluationContext curr_eval_ctx(
            curr_state, succ_g, false, &statistics);
        int curr_eval = curr_eval_ctx.get_evaluator_value_or_infinity(evaluator.get());

        bool keep_open = false;
        if (curr_eval < last_eval && !found_hi) {

            state_depth[curr_state] = rollout_eval;
            types[rollout_eval].push_back(curr_state.get_id());

            current_sum += std::exp(-1.0*static_cast<double>(rollout_eval) / tau);

            // open_list->notify_state_transition(last_state, next_op, curr_state);
            // open_list->insert(curr_eval_ctx, curr_state.get_id());
            keep_open == true;
            found_hi = true;
        } 
        if (curr_eval < rollout_eval) {

            if (!keep_open) {
                state_depth[curr_state] = curr_eval;//rollout_depth+1;
                types[state_depth[curr_state]].push_back(curr_state.get_id());

                current_sum += std::exp(-1.0*static_cast<double>(state_depth[curr_state]) / tau);
            }
            // open_list->notify_state_transition(last_state, next_op, curr_state);
            // open_list->insert(curr_eval_ctx, curr_state.get_id());
            rollout_budget = budget;
            // return iterated_rollout(curr_state, limit);
        }

        next_op = random_next_action(rollout_state);
        if (next_op == OperatorID::no_operator) {
            return IN_PROGRESS;
        }
        op = task_proxy.get_operators()[next_op];
        State succ_state = state_registry.get_successor_state(curr_state, op);
        statistics.inc_generated();

        if(search_space.get_node(curr_state).is_open() && !keep_open)
            search_space.get_node(curr_state).close();

        SearchNode succ_node = search_space.get_node(succ_state);
        if (succ_node.is_new())
            succ_node.open(curr_node, op, get_adjusted_cost(op));
        last_state = curr_state;
        last_eval = curr_eval;
        curr_state = succ_state;

        if (search_progress.check_progress(curr_eval_ctx)) {
            statistics.print_checkpoint_line(succ_node.get_g());
            // reward_progress();
        }

    rollout_budget-=1;
    } while(rollout_budget > 0);

    return  IN_PROGRESS;

}

inline State UHRSearch::random_policy(State parent_state, State parent_prune_state) {

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(parent_state, applicable_ops);
    int i = rng->random(applicable_ops.size());
    OperatorID op_id = applicable_ops[i];
    OperatorProxy curr_op = task_proxy.get_operators()[op_id];
    State chosen_s = state_registry.get_successor_state(parent_state, curr_op);

    if (chosen_s == parent_prune_state) { 
        i = (i+1) % applicable_ops.size();

        op_id = applicable_ops[i];
        curr_op = task_proxy.get_operators()[op_id];
        chosen_s = state_registry.get_successor_state(parent_state, curr_op);
    }

    return chosen_s;

}

SearchStatus UHRSearch::step() {

    tl::optional<SearchNode> node;
    while (true) {
        if (types.empty()) {
            log << "Completely explored state space -- no solution!" << endl;
            return FAILED;
        }

        int selected_depth = types.begin()->first;
        if (types.size() > 1) {
            double r = rng->random();
            
            double total_sum = current_sum;
            double p_sum = 0.0;
            for (auto it : types) {
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

        vector<StateID> &states = types.at(selected_depth);
        int chosen_i = rng->random(states.size());
        StateID id = states[chosen_i];
        utils::swap_and_pop_from_vector(states, chosen_i);
        if (states.empty()){
            types.erase(selected_depth);
            current_sum -= std::exp(-1.0*static_cast<double>(selected_depth) / tau);
        }

        State s = state_registry.lookup_state(id);
        node.emplace(search_space.get_node(s));

        if (node->is_closed())
            continue;

        node->close();
        assert(!node->is_dead_end());
        statistics.inc_expanded();
        break;
    }

    const State &s = node->get_state();
    // if (check_goal_and_set_plan(s)) //checked at generation
    //     return SOLVED;

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);

    /*
      TODO: When preferred operators are in use, a preferred operator will be
      considered by the preferred operator queues even when it is pruned.
    */
    // pruning_method->prune_operators(s, applicable_ops);

    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node->get_g(), false, &statistics, true);
    int parent_eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    // int parent_depth = state_depth[s];

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        // if ((node->get_real_g() + op.get_cost()) >= bound)
        //     continue;

        State succ_state = state_registry.get_successor_state(s, op);
        statistics.inc_generated();
        // bool is_preferred = preferred_operators.contains(op_id);

        SearchNode succ_node = search_space.get_node(succ_state);

        // // notify path dependent evaluators of transition
        // for (Evaluator *evaluator : path_dependent_evaluators) {
        //     evaluator->notify_state_transition(s, op_id, succ_state);
        // }

        // notify path dependent openlist of transition
        // open_list->notify_state_transition(s, op_id, succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.
            int succ_g = node->get_g() + get_adjusted_cost(op);

            EvaluationContext succ_eval_context(
                succ_state, succ_g, false, &statistics);
            statistics.inc_evaluated_states();

            // if (open_list->is_dead_end(succ_eval_context)) {
            //     succ_node.mark_as_dead_end();
            //     statistics.inc_dead_ends();
            //     continue;
            // }
            succ_node.open(*node, op, get_adjusted_cost(op));

            int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());
            if (eval < parent_eval) {
                state_depth[succ_state] = eval;
                current_sum += std::exp(-1.0*static_cast<double>(state_depth[succ_state]) / tau);
                types[state_depth[succ_state]].push_back(succ_state.get_id());
            } else {
                state_depth[succ_state] = parent_eval;
                types[parent_eval].push_back(succ_state.get_id());
            }


            // open_list->insert(succ_eval_context, succ_state.get_id());
            if (search_progress.check_progress(succ_eval_context)) {
                statistics.print_checkpoint_line(succ_node.get_g());
                // reward_progress();
            }

            if (check_goal_and_set_plan(succ_state)) {
                return SOLVED;
            }
            if (iterated_rollout(succ_state, r_limit) == SOLVED) {
                return SOLVED;
            }

        } else if (succ_node.get_g() > node->get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                if (succ_node.is_closed()) {
                    /*
                      TODO: It would be nice if we had a way to test
                      that reopening is expected behaviour, i.e., exit
                      with an error when this is something where
                      reopening should not occur (e.g. A* with a
                      consistent heuristic).
                    */
                    statistics.inc_reopened();
                }
                succ_node.reopen(*node, op, get_adjusted_cost(op));

                EvaluationContext succ_eval_context(
                    succ_state, succ_node.get_g(), false, &statistics);
                
                int eval = succ_eval_context.get_evaluator_value_or_infinity(evaluator.get());
                if (eval < parent_eval) {
                    state_depth[succ_state] = eval;
                    current_sum += std::exp(-1.0*static_cast<double>(eval) / tau);
                    types[eval].push_back(succ_state.get_id());
                } else {
                    state_depth[succ_state] = parent_eval;
                    types[parent_eval].push_back(succ_state.get_id());
                }

                // open_list->insert(succ_eval_context, succ_state.get_id());
                if (iterated_rollout(succ_state, r_limit) == SOLVED) {
                    return SOLVED;
                }
            } else {
                // If we do not reopen closed nodes, we just update the parent pointers.
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back.
                succ_node.update_parent(*node, op, get_adjusted_cost(op));
            }
        }
    }
    next_type+=1;

    return IN_PROGRESS;
}

void UHRSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    // open_list->boost_preferred();
}

void UHRSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void UHRSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    // if (f_evaluator) {
    //     int f_value = eval_context.get_evaluator_value(f_evaluator.get());
    //     statistics.report_f_value_progress(f_value);
    // }
}

/* TODO: HACK! This is very inefficient for simply looking up an h value.
   Also, if h values are not saved it would recompute h for each and every state. */
void UHRSearch::update_f_value_statistics(EvaluationContext &eval_context) {
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
