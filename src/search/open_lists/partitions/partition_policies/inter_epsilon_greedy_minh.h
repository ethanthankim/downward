#ifndef PARTITION_POLICIES_INTER_EG_MINH_H
#define PARTITION_POLICIES_INTER_EG_MINH_H

#include "partition_policy.h"

#include "../../utils/rng.h"
#include "../../utils/rng_options.h"

#include <map>

namespace inter_eg_minh_partition {
class InterEpsilonGreedyMinHPolicy : public PartitionPolicy {

    shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;

    using Partition = vector<Key>;
    unordered_map<Key, pair<int, Partition>> partition_buckets;
    vector<Key> partition_heap; 

private: 
    using Synchronizer = void (InterEpsilonGreedyMinHPolicy::*)(vector<Key>&, int, int);
    using Comparer = bool (InterEpsilonGreedyMinHPolicy::*)(vector<Key>&, int, int, utils::HashMap<Key, PartitionedState>);
    void adjust_heap_down(vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> active_states, Comparer, Synchronizer = NULL); 
    void adjust_heap_up(vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> active_states, Comparer, Synchronizer = NULL);   
    void adjust_to_top(vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> active_states, Synchronizer = NULL); 
    Key random_access_heap_pop(vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> active_states, Comparer, Synchronizer = NULL);

    bool compare_parent_node_smaller(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states);
    bool compare_parent_node_bigger(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states);
    bool compare_parent_type_smaller(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states);
    bool compare_parent_type_bigger(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states);
    void sync_type_heap_and_type_location(vector<Key> &heap, int pos1, int pos2);

    inline int state_h(int state_id, utils::HashMap<Key, PartitionedState> active_states);

public:
    explicit InterEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~InterEpsilonGreedyMinHPolicy() override = default;

    virtual Key remove_min() override;
    virtual void notify_insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states) override;
    virtual void notify_remove(Key inserted, utils::HashMap<Key, PartitionedState> active_states) override;
};
}

#endif