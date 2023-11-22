#include "lwm_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"
#include <algorithm> 

using namespace std;

namespace lwm_evaluator {
LWMEvaluator::LWMEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void LWMEvaluator::notify_initial_state(const State &initial_state) {
    parent_h = INT32_MAX;
}

void LWMEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_h = h_cache[parent_state];
}

EvaluationResult LWMEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    result.set_evaluator_value(min(new_h, parent_h));
    return result;
    
}


class LWMEvaluatorFeature : public plugins::TypedFeature<Evaluator, LWMEvaluator> {
public:
    LWMEvaluatorFeature() : TypedFeature("lwm") {
        document_subcategory("evaluators_basic");
        document_title("Low water-mark evaluator");
        document_synopsis(
            "Return minimum between parent and child.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<LWMEvaluatorFeature> _plugin;
}
