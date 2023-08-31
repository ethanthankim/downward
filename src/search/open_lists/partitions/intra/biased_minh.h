#ifndef NODE_POLICIES_INTRA_BIASED
#define NODE_POLICIES_INTRA_BIASED

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>
#include <deque>

namespace intra_partition_biased {
class IntraBiasedPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::map<int, std::deque<NodeKey>> buckets;
    utils::HashMap<NodeKey, int> partition_indices;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;
 
    NodeKey cached_last_removed = -1;
    
public:
    explicit IntraBiasedPolicy(const plugins::Options &opts);
    virtual ~IntraBiasedPolicy() override = default;

    virtual void insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) override;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) override;
};
}

#endif