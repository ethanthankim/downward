#ifndef PARTITION_POLICIES_UNIFORM_H
#define PARTITION_POLICIES_UNIFORM_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <memory>
#include <map>

namespace inter_uniform_partition {
class InterUniformPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    utils::HashMap<int, int> partition_sizes;

public:
    explicit InterUniformPolicy(const plugins::Options &opts);
    virtual ~InterUniformPolicy() override = default;

    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual void notify_removal(int partition_key, int node_key) {
        partition_sizes[partition_key]-=1;
    };
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        partition_sizes.clear();
    };
};
}

#endif