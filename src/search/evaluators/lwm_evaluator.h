#ifndef EVALUATORS_HI_EVALUATOR_H
#define EVALUATORS_HI_EVALUATOR_H

#include "../evaluator.h"
#include "../per_state_information.h"

namespace lwm_evaluator {
class LWMEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;

    PerStateInformation<int> h_cache;
    int parent_h;

public:
    explicit LWMEvaluator(const plugins::Options &opts);
    virtual ~LWMEvaluator() override = default;

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