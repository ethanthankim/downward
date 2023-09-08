#include "random_range_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../plugins/plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace random_range_evaluator {
RandomRangeEvaluator::RandomRangeEvaluator(const plugins::Options &opts)
    : Evaluator(opts),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      rng(utils::parse_rng_from_options(opts)) {
}

RandomRangeEvaluator::~RandomRangeEvaluator() {
}

bool RandomRangeEvaluator::dead_ends_are_reliable() const {
    return evaluator->dead_ends_are_reliable();
}

EvaluationResult RandomRangeEvaluator::compute_result(
    EvaluationContext &eval_context) {
    EvaluationResult result;
    int value = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (value != EvaluationResult::INFTY) {
        value = rng->random(value+1);
    }
    result.set_evaluator_value(value);
    return result;
}

void RandomRangeEvaluator::get_path_dependent_evaluators(set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

class RandomRangeEvaluatorFeature : public plugins::TypedFeature<Evaluator, RandomRangeEvaluator> {
public:
    RandomRangeEvaluatorFeature() : TypedFeature("rand_range") {
        document_subcategory("evaluators_basic");
        document_title("Random Range evaluator");
        document_synopsis(
            "For a given evaluation h, returns a uniform random value between [0, h]");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<RandomRangeEvaluatorFeature> _plugin;
}
