#ifndef EVALUATORS_PROGRESS_RATE_EVALUATOR_H
#define EVALUATORS_PROGRESS_RATE_EVALUATOR_H

#include "../evaluator.h"
#include "../per_state_information.h"

namespace progress_rate_evaluator {
class ProgressRateEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;

    using Progress = int;
    using H = int;
    PerStateInformation<std::pair<H, Progress>> eval_cache;
    std::pair<H, Progress> parent_cache;

public:
    explicit ProgressRateEvaluator(const plugins::Options &opts);
    virtual ~ProgressRateEvaluator() override = default;

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
