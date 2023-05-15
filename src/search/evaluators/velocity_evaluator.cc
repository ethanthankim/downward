#include "velocity_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace velocity_evaluator {
VelocityEvaluator::VelocityEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void VelocityEvaluator::notify_initial_state(const State &initial_state) {
    parent_cache = make_pair(INT32_MAX, 0);
}

void VelocityEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_cache = eval_cache[parent_state];
}

EvaluationResult VelocityEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    H parent_h = parent_cache.first;

    int succ_quant = new_h < parent_h ? parent_cache.second+(parent_h - new_h) : 0;
    double succ_rate = (double) (succ_quant / eval_context.get_g_value());
    eval_cache[eval_context.get_state()] = make_pair(new_h, succ_quant);
    
    int new_eval = (int) 100 * succ_rate;
    result.set_evaluator_value(new_eval);
    return result;
}

class VelocityEvaluatorFeature : public plugins::TypedFeature<Evaluator, VelocityEvaluator> {
public:
    VelocityEvaluatorFeature() : TypedFeature("vel") {
        document_subcategory("evaluators_basic");
        document_title("Velocity evaluator");
        document_synopsis(
            "Return the velocity from initial state to source node.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<VelocityEvaluatorFeature> _plugin;
}
