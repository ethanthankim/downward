#ifndef SEARCH_ENGINES_UHR_SEARCH_H
#define SEARCH_ENGINES_UHR_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"
#include "../utils/rng.h"

#include <memory>
#include <vector>
#include <map>

class Evaluator;
class PruningMethod;

namespace plugins {
class Feature;
}

namespace uhr_search {
class UHRSearch : public SearchEngine {
    const bool reopen_closed_nodes;

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    
    utils::HashMap<int, int> type_roll_len;
    PerStateInformation<int> state_to_type;
    std::map<int, std::vector<StateID>> types;
    int next_type = 0;
    double tau;
    int r_limit;
    double current_sum;
    int budget;

    struct RolloutState {
        State state;
        int g_cost;
        OperatorID gen_id;

        RolloutState(State &state, int g_cost, OperatorID gen_id) :
            state(state), g_cost(g_cost), gen_id(gen_id) {};
    };

    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(EvaluationContext &eval_context);
    void reward_progress();

protected:
    virtual void initialize() override;
    OperatorID random_next_action(State s);
    SearchStatus greedy_rollout(State rollout_state);
    inline int greedy_policy(vector<EvaluationContext>& succ_eval);
    SearchStatus UHRSearch::random_rollout(State rollout_state, State parent_state);
    inline State random_policy(State parent_state, State parent_prune_state);
    virtual SearchStatus step() override;

public:
    explicit UHRSearch(const plugins::Options &opts);
    virtual ~UHRSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
