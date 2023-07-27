/* -*- mode : c++ -*- */
#pragma once

#include "../evaluator.h"
#include <vector>
#include <utility>
#include <map>
#include "../plugins/plugin.h"
#include "../per_state_information.h"
#include "../task_proxy.h"
#include "../utils/rng.h"

class Heuristic;
class Options;

namespace RandomEdgeEvaluator {
    class RandomEdgeEvaluator : public Evaluator {
        PerStateInformation<int> state_db;
        std::map<const OperatorProxy*,int> edge_db;
        int bound;
        shared_ptr<utils::RandomNumberGenerator> rng;
    public:
        explicit RandomEdgeEvaluator(const plugins::Options &options);
        virtual ~RandomEdgeEvaluator() override = default;

        virtual EvaluationResult compute_result(
            EvaluationContext &eval_context) override;
    };
}
