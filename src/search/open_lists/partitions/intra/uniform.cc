#include "uniform.h"

using namespace std;

namespace intra_uniform_partition {

IntraUniformPolicy::IntraUniformPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}

int IntraUniformPolicy::get_next_node(int partition_key){ 

    vector<int> &partition = node_partitions.at(partition_key);
    int rand_i = rng->random(partition.size());
    auto ret = utils::swap_and_pop_from_vector(partition, rand_i);
    
    if (partition.size()==0)
        node_partitions.erase(partition_key);

    return ret;
}

void IntraUniformPolicy::notify_insert(
    int partition_key,
    int node_key,
    bool new_partition,
    int eval) 
{
    node_partitions[partition_key].push_back(node_key);
}

class IntraUniformPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraUniformPolicy> {
public:
    IntraUniformPolicyFeature() : TypedFeature("intra_uniform") {
        document_subcategory("node_policies");
        document_title("Uniform Random node selection");
        document_synopsis(
            "Uniformly at random choose the next node from the partition");
        utils::add_rng_options(*this);
        add_node_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraUniformPolicyFeature> _plugin;
}