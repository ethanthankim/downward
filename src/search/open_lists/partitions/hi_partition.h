#ifndef PARTITIONS_HI_PARTITION_H
#define PARTITIONS_HI_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace hi_partition {
class HIPartition : public PartitionSystem {

    StateID cached_parent_id = StateID::no_state;
    StateID cached_next_state_id = StateID::no_state;
public:
    explicit HIPartition(const plugins::Options &opts);
    virtual ~HIPartition() override = default;

    Key choose_state_partition(utils::HashMap<Key, PartitionedState> active_states) override;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif