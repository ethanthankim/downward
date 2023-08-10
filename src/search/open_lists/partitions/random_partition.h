#ifndef PARTITIONS_RANDOM_PARTITION_H
#define PARTITIONS_RANDOM_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace random_partition {
class RandomPartition : public PartitionSystem {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;
    Key last_partition = -1;

    utils::HashMap<Key, std::vector<Key>> partitions;

public:
    explicit RandomPartition(const plugins::Options &opts);
    virtual ~RandomPartition() override = default;

    virtual Key insert_state(Key to_insert, const utils::HashMap<Key, OpenState> &open_states) override;
    virtual Key select_next_partition(const utils::HashMap<Key, OpenState> &open_states) override;
    virtual Key select_next_state_from_partition(Key partition, const utils::HashMap<Key, OpenState> &open_states) override;

};
}

#endif