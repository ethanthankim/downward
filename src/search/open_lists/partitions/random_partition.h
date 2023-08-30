#ifndef PARTITIONS_RANDOM_PARTITION_H
#define PARTITIONS_RANDOM_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace random_partition {
class RandomPartition : public PartitionSystem {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::shared_ptr<PartitionSystem> partition_system;
    double epsilon_use_partition;
    double epsilon_can_distinguish;
    StateID cached_parent_id = StateID::no_state;
    StateID cached_next_state_id = StateID::no_state;

public:
    explicit RandomPartition(const plugins::Options &opts);
    virtual ~RandomPartition() override = default;

    std::pair<bool, PartitionKey> choose_state_partition(
        utils::HashMap<NodeKey, PartitionedState> active_states,
        utils::HashMap<PartitionKey, Partition> partition_buckets) override;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif