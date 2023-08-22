#ifndef PARTITIONS_LWM_PARTITION_H
#define PARTITIONS_LWM_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace lwm_partition {
class LWMPartition : public PartitionSystem {

    StateID cached_parent_id = StateID::no_state;
    StateID cached_next_state_id = StateID::no_state;
    utils::HashMap<PartitionKey, int> lwm_values;

public:
    explicit LWMPartition(const plugins::Options &opts);
    virtual ~LWMPartition() override = default;

    std::pair<bool, PartitionKey> choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states) override;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif