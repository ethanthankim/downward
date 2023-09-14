#ifndef EVALUATORS_PROGRESS_EVALUATOR_H
#define EVALUATORS_PROGRESS_EVALUATOR_H

#include "../evaluator.h"
#include "../per_state_information.h"

namespace progress_evaluator {
class ProgressEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;

    PerStateInformation<int> eval_cache;
    int cached_parent_h;

public:
    explicit ProgressEvaluator(const plugins::Options &opts);
    virtual ~ProgressEvaluator() override = default;

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
