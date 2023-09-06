#include "uniform.h"

using namespace std;

namespace intra_partition_uniform {

IntraUniformPolicy::IntraUniformPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}

int IntraUniformPolicy::get_next_node(int partition_key){ 
    
    if (node_partitions.count(last_chosen_partition)>0 && node_partitions.at(last_chosen_partition).size()==0)
        node_partitions.erase(last_chosen_partition);

    last_chosen_partition = partition_key;
    vector<int> &partition = node_partitions.at(partition_key);
    auto ret = utils::swap_and_pop_from_vector(partition, rng->random(partition.size()));
    // log << "[get_next_node] selected node: " << ret << endl;
    return ret;
}

void IntraUniformPolicy::notify_insert(
    int partition_key,
    int node_key,
    bool new_partition,
    EvaluationContext &eval_context) 
{
    if (new_partition) {
        // log << "[node notify_insert] new partition: " << partition_key  << endl;
        node_partitions.emplace(partition_key, vector<int>({node_key}));
    } else {
        node_partitions.at(partition_key).push_back(node_key);
    }
    // log << "[node notify_insert] node inserted: " << node_key  << endl;
    // log << "[node notify_insert] partition size: " << partition_key << " - " << node_partitions.at(partition_key).size() << endl;
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