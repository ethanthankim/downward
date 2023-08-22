#include "uniform.h"

using namespace std;

namespace intra_partition_uniform {

IntraUniformPolicy::IntraUniformPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}

NodeKey IntraUniformPolicy::remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) { 
    
    active_states.erase(cached_last_removed);
    int pos = rng->random(partition.size());

    NodeKey to_return = utils::swap_and_pop_from_vector(partition, pos);
    cached_last_removed = to_return;
    return to_return;
}

void IntraUniformPolicy::insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) {
    partition.push_back(inserted);
}

class IntraUniformPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraUniformPolicy> {
public:
    IntraUniformPolicyFeature() : TypedFeature("intra_uniform") {
        document_subcategory("node_policies");
        document_title("Uniform Random node selection");
        document_synopsis(
            "Uniformly at random choose the next node from the partition");
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraUniformPolicyFeature> _plugin;
}