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
    cached_initial = make_pair(initial_state.get_id().get_value(), 0);
}

// void VelocityEvaluator::notify_state_transition(
//     const State &parent_state, OperatorID op_id, const State &state) {
//     parent_cache = eval_cache[parent_state];
// }

EvaluationResult VelocityEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    int new_id = eval_context.get_state().get_id().get_value();
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (new_id == cached_initial.first) {
        // special case for initial node insertion
        cached_initial.second = new_h;
    }

    int g = eval_context.get_g_value() == 0 ? 1 : eval_context.get_g_value();
    int initial_h = cached_initial.second;
    double avg_velocity = (initial_h - new_h) / g;
    
    // int new_eval = initial_h - avg_velocity;
    result.set_evaluator_value(initial_h - avg_velocity);
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
