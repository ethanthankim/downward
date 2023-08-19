#ifndef NODE_POLICIES_INTRA_EG_MINH_H
#define NODE_POLICIES_INTRA_EG_MINH_H

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace intra_partition_eg_minh {
class IntraEpsilonGreedyMinHPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;
 
    NodeKey cached_last_removed = -1;

private:
    bool compare_parent_node_smaller(NodeKey parent, NodeKey child, utils::HashMap<NodeKey, PartitionedState> & active_states);
    bool compare_parent_node_bigger(NodeKey parent, NodeKey child, utils::HashMap<NodeKey, PartitionedState> & active_states);
    void adjust_to_top(std::vector<NodeKey> &heap, int pos);
    void adjust_heap_down(std::vector<NodeKey> &heap, int loc, utils::HashMap<NodeKey, PartitionedState> &active_states);
    void adjust_heap_up(std::vector<NodeKey> &heap, int pos, utils::HashMap<NodeKey, PartitionedState> &active_states);
    NodeKey random_access_heap_pop(std::vector<NodeKey> &heap, int loc, utils::HashMap<NodeKey, PartitionedState> &active_states);

public:
    explicit IntraEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~IntraEpsilonGreedyMinHPolicy() override = default;

    virtual void insert(NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) override;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) override;
};
}

#endif