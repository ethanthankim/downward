#ifndef PARTITIONS_PLATEAU_PARTITION_H
#define PARTITIONS_PLATEAU_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace plateau_partition {
class PlateauPartition : public PartitionSystem {

    StateID cached_next_state_id = StateID::no_state;

public:
    explicit PlateauPartition(const plugins::Options &opts);
    virtual ~PlateauPartition() override = default;

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