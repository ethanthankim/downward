#ifndef SEARCH_ENGINES_NEW_SEARCH_H
#define SEARCH_ENGINES_NEW_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"
#include "../utils/rng.h"
#include "../task_utils/task_properties.h"

#include <memory>
#include <vector>
#include <map>

class Evaluator;
class PruningMethod;

namespace plugins {
class Feature;
}

namespace new_search {
class NewSearch : public SearchEngine {
    const bool reopen_closed_nodes;

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    // std::unique_ptr<StateOpenList> open_list;

    // struct StateType {
    //     int type;
    //     int eval;
    //     StateType(int type, int eval) : 
    //         type(type), eval(eval) {}
    //     StateType() : type(0), eval(std::numeric_limits<int>::max()) {}
    // };
    // PerStateInformation<StateType> state_info;
    // std::map<int, utils::HashMap<int, std::vector<StateID>>, std::greater<int>> types;
    
    enum RolloutResult { UHR, GOAL };
    struct OpenState {
        int depth;
        int op_index;
        bool greedily_expanded;
        std::vector<OperatorID> operators;
        OpenState(int depth, int op_index, bool greedily_expanded, std::vector<OperatorID> &operators)
            : depth(depth), op_index(op_index), greedily_expanded(greedily_expanded), operators(operators) {
        }
        OpenState() :
            depth(0), op_index(-1) {} 
    };
    PerStateInformation<OpenState> open_states;
    std::map<int, std::vector<std::vector<StateID>>, std::greater<int>> hi_types;
    double tau;
    double current_sum;
    bool initial_expansion;

    // the 'type' segments along a rollout for easy addition to type system
    std::vector<std::vector<State>> active_rollout_path_segments;

    // std::shared_ptr<Evaluator> f_evaluator;

    // std::vector<Evaluator *> path_dependent_evaluators;
    // std::vector<std::shared_ptr<Evaluator>> preferred_operator_evaluators;
    // std::shared_ptr<Evaluator> lazy_evaluator;

    // std::shared_ptr<PruningMethod> pruning_method;

    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(EvaluationContext &eval_context);
    void reward_progress();

protected:
    virtual void initialize() override;
    OperatorID random_next_action(State s);
    OperatorID get_next_rollout_start(State s);
    bool open_rollout_node(State s, OperatorID op_id, State parent_s);
    // SearchStatus iterated_rollout(State rollout_root, int r_limit);
    RolloutResult greedy_rollout(const State rollout_state);
    RolloutResult random_rollout(const State rollout_state, int start_h, int rollout_limit);
    bool partially_expand_state(SearchNode& expand_state);
    virtual SearchStatus step() override;

public:
    explicit NewSearch(const plugins::Options &opts);
    virtual ~NewSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
