#ifndef EVALUATORS_RANDOM_RANGE_EVALUATOR_H
#define EVALUATORS_RANDOM_RANGE_EVALUATOR_H

#include "../evaluator.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <random>
#include <memory>

namespace plugins {
class Options;
}

namespace random_range_evaluator {
class RandomRangeEvaluator : public Evaluator {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::shared_ptr<Evaluator> evaluator;

public:
    explicit RandomRangeEvaluator(const plugins::Options &opts);
    virtual ~RandomRangeEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
