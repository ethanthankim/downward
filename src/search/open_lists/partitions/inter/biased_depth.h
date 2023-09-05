#ifndef PARTITION_POLICIES_BIASED_DEPTH_H
#define PARTITION_POLICIES_BIASED_DEPTH_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_depth_partition {
class InterBiasedDepthPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;

    int last_chosen_partition_i = -1;
    int last_chosen_depth = -1;

    std::map<int, std::vector<std::pair<int, int>>> buckets;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;

public:
    explicit InterBiasedDepthPolicy(const plugins::Options &opts);
    virtual ~InterBiasedDepthPolicy() override = default;

    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual void notify_removal(int partition_key, int node_key) {};
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        last_chosen_partition_i = -1;
        last_chosen_depth = -1;
        buckets.clear();
    };
};
}

#endif