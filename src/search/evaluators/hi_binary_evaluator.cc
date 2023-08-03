#include "hi_binary_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace hi_binary_evaluator {
HIBinEvaluator::HIBinEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void HIBinEvaluator::notify_initial_state(const State &initial_state) {
    parent_cache = make_pair(INT32_MAX, 0);
}

void HIBinEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_cache = eval_cache[parent_state];
}

EvaluationResult HIBinEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    H new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    Type new_type = eval_context.get_state().get_id().get_value();
    H parent_h = parent_cache.first;
    Type parent_type = parent_cache.second;

    if (new_h != parent_h) {
        result.set_evaluator_value(1);
    } else {
        result.set_evaluator_value(0);
    }

    return result;
}


class HIBinEvaluatorFeature : public plugins::TypedFeature<Evaluator, HIBinEvaluator> {
public:
    HIBinEvaluatorFeature() : TypedFeature("hi_bin") {
        document_subcategory("evaluators_basic");
        document_title("Progress rate evaluator");
        document_synopsis(
            "Return the rate of progress from initial state to source node.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<HIBinEvaluatorFeature> _plugin;
}
