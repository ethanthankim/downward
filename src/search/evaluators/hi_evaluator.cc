#include "hi_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace hi_evaluator {
HIEvaluator::HIEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void HIEvaluator::notify_initial_state(const State &initial_state) {
    parent_cache = make_pair(INT32_MAX, 0);
}

void HIEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_cache = eval_cache[parent_state];
}

EvaluationResult HIEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    H new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    Type new_type = eval_context.get_state().get_id().get_value();
    H parent_h = parent_cache.first;
    Type parent_type = parent_cache.second;

    if (new_h == parent_h) {
        result.set_evaluator_value(new_type);
    } else {
        result.set_evaluator_value(parent_type);
    }

    return result;
}


class HIEvaluatorFeature : public plugins::TypedFeature<Evaluator, HIEvaluator> {
public:
    HIEvaluatorFeature() : TypedFeature("hi") {
        document_subcategory("evaluators_basic");
        document_title("Progress rate evaluator");
        document_synopsis(
            "Return the rate of progress from initial state to source node.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<HIEvaluatorFeature> _plugin;
}
