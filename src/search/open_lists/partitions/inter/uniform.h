#ifndef PARTITION_POLICIES_UNIFORM_H
#define PARTITION_POLICIES_UNIFORM_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_uniform_partition {
class InterUniformPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
 
    PartitionKey cached_parent_type = -1;

public:
    explicit InterUniformPolicy(const plugins::Options &opts);
    virtual ~InterUniformPolicy() override = default;

    virtual PartitionKey get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override;
    virtual void notify_insert(bool new_type, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override {};
    virtual void notify_remove(NodeKey removed, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) override {};
};
}

#endif