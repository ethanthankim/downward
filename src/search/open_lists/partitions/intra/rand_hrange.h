#ifndef NODE_POLICIES_INTRA_EG_HRANGE_H
#define NODE_POLICIES_INTRA_EG_HRANGE_H

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace intra_partition_eg_hrange {
class IntraEpsilonGreedyHRangePolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;

    utils::HashMap<NodeKey, int> rand_hs;
    NodeKey cached_last_removed = -1;

private:
    bool compare_parent_node_smaller(NodeKey parent, NodeKey child);
    bool compare_parent_node_bigger(NodeKey parent, NodeKey child);
    void adjust_to_top(std::vector<NodeKey> &heap, int pos);
    void adjust_heap_down(std::vector<NodeKey> &heap, int loc);
    void adjust_heap_up(std::vector<NodeKey> &heap, int pos);
    NodeKey random_access_heap_pop(std::vector<NodeKey> &heap, int loc);

public:
    explicit IntraEpsilonGreedyHRangePolicy(const plugins::Options &opts);
    virtual ~IntraEpsilonGreedyHRangePolicy() override = default;

    virtual void insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, Partition &partition) override;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, Partition &partition) override;
};
}

#endif