#ifndef PARTITION_POLICIES_INTER_EG_MINH_H
#define PARTITION_POLICIES_INTER_EG_MINH_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_eg_minh_partition {
class InterEpsilonGreedyMinHPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;

    std::vector<Key> partition_heap; 
    Key cached_parent_type = -1;
    int cached_partition_position = 0;

private: 
    void adjust_heap_down(int loc, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets); 
    void adjust_heap_up(int pos, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets);   
    void adjust_to_top(int loc); 
    Key random_access_heap_pop(int loc, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets);

    bool compare_parent_type_smaller(int parent, int child, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets);
    bool compare_parent_type_bigger(int parent, int child, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets);

    inline int state_h(int state_id, utils::HashMap<Key, PartitionedState> &active_states);

public:
    explicit InterEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~InterEpsilonGreedyMinHPolicy() override = default;

    virtual Key remove_min(utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets) override;
    virtual void notify_insert(Key inserted, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets) override;
    virtual void notify_remove(Key removed, utils::HashMap<Key, PartitionedState> &active_states, utils::HashMap<Key, Partition> &partition_buckets) override {};
};
}

#endif