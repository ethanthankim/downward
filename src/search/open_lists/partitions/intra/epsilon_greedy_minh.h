#ifndef NODE_POLICIES_INTER_EG_MINH_H
#define NODE_POLICIES_INTER_EG_MINH_H

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace intra_partition_eg_minh {
class IntraEpsilonGreedyMinHPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;
 
    Key cached_last_removed = -1;

private:
    bool compare_parent_node_smaller(Key parent, Key child, utils::HashMap<Key, PartitionedState> & active_states);
    bool compare_parent_node_bigger(Key parent, Key child, utils::HashMap<Key, PartitionedState> & active_states);
    void adjust_to_top(std::vector<Key> &heap, int pos);
    void adjust_heap_down(std::vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> &active_states);
    void adjust_heap_up(std::vector<Key> &heap, int pos, utils::HashMap<Key, PartitionedState> &active_states);
    Key random_access_heap_pop(std::vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> &active_states);

public:
    explicit IntraEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~IntraEpsilonGreedyMinHPolicy() override = default;

    virtual void insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states, std::vector<Key> &partition) override;
    virtual Key remove_next_state_from_partition(utils::HashMap<Key, PartitionedState> &active_states, std::vector<Key> &partition) override;
};
}

#endif