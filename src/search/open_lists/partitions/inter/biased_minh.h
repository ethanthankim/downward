#ifndef PARTITION_POLICIES_BIASED_H
#define PARTITION_POLICIES_BIASED_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_partition {
class InterBiasedPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;

    PartitionKey last_chosen_partition_key = -1;
    int last_chosen_partition_i = -1;
    int last_chosen_h = -1;

    std::map<int, std::vector<PartitionKey>> h_buckets;
    int size;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double epsilon;
    double current_sum;

public:
    explicit InterBiasedPolicy(const plugins::Options &opts);
    virtual ~InterBiasedPolicy() override = default;

    virtual PartitionKey get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override;
    virtual void notify_insert(bool new_type, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override;
    virtual void notify_remove(NodeKey removed, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override;
};
}

#endif