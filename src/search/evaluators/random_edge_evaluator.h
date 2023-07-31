#ifndef EVALUATORS_RANDOM_EDGE_EVALUATOR_H
#define EVALUATORS_RANDOM_EDGE_EVALUATOR_H

#include "../evaluator.h"
#include <vector>
#include <utility>
#include <map>
#include "../plugins/plugin.h"
#include "../per_state_information.h"
#include "../task_proxy.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace random_edge_evaluator {
class RandomEdgeEvaluator : public Evaluator {
    PerStateInformation<int> state_db;
    std::map<const OperatorProxy*,int> edge_db;
    int bound;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    OperatorID creating_op = OperatorID::no_operator;
    StateID created_state_id = StateID::no_state;
public:
    explicit RandomEdgeEvaluator(const plugins::Options &options);
    virtual ~RandomEdgeEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) override;
};
}

#endif
