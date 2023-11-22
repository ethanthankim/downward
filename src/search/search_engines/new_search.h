#ifndef SEARCH_ENGINES_EAGER_SEARCH_H
#define SEARCH_ENGINES_EAGER_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"
#include "../utils/rng.h"

#include <memory>
#include <vector>

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
    std::unique_ptr<StateOpenList> open_list;
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
    SearchStatus iterated_rollout(State rollout_root);
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
