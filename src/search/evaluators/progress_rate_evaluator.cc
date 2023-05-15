#include "progress_rate_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

using namespace std;

namespace progress_rate_evaluator {
ProgressRateEvaluator::ProgressRateEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

void ProgressRateEvaluator::notify_initial_state(const State &initial_state) {
    parent_cache = make_pair(INT32_MAX, 0);
}

void ProgressRateEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_cache = eval_cache[parent_state];
}

EvaluationResult ProgressRateEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    H parent_h = parent_cache.first;

    int succ_quant = new_h < parent_h ? parent_cache.second+(parent_h - new_h) : 0;
    double succ_rate = (double) ( (succ_quant + 1) / (eval_context.get_g_value() + 2) );
    eval_cache[eval_context.get_state()] = make_pair(new_h, succ_quant);
    
    int new_eval = (int) 1000 * (1-succ_rate); // 1-succ_rate because min heap
    result.set_evaluator_value(new_eval);
    return result;
}


class ProgressRateEvaluatorFeature : public plugins::TypedFeature<Evaluator, ProgressRateEvaluator> {
public:
    ProgressRateEvaluatorFeature() : TypedFeature("prog") {
        document_subcategory("evaluators_basic");
        document_title("Progress rate evaluator");
        document_synopsis(
            "Return the rate of progress from initial state to source node.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<ProgressRateEvaluatorFeature> _plugin;
}
