#include "progress_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace progress_evaluator {
ProgressEvaluator::ProgressEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void ProgressEvaluator::notify_initial_state(const State &initial_state) {
    cached_parent_h = numeric_limits<int>::max();
}

void ProgressEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    cached_parent_h = eval_cache[parent_state];
}

EvaluationResult ProgressEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    eval_cache[eval_context.get_state()] = new_h;

    int new_eval = new_h - cached_parent_h;
    result.set_evaluator_value(new_eval);
    return result;
}


class ProgressEvaluatorFeature : public plugins::TypedFeature<Evaluator, ProgressEvaluator> {
public:
    ProgressEvaluatorFeature() : TypedFeature("prog") {
        document_subcategory("evaluators_basic");
        document_title("Progress rate evaluator");
        document_synopsis(
            "Return the amount of progress from parent to child.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<ProgressEvaluatorFeature> _plugin;
}
