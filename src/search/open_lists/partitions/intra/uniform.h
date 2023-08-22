#ifndef NODE_POLICIES_INTRA_UNIFORM
#define NODE_POLICIES_INTRA_UNIFORM

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace intra_partition_uniform {
class IntraUniformPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;
 
    NodeKey cached_last_removed = -1;
    
public:
    explicit IntraUniformPolicy(const plugins::Options &opts);
    virtual ~IntraUniformPolicy() override = default;

    virtual void insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) override;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) override;
};
}

#endif