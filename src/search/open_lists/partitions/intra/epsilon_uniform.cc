#include "epsilon_uniform.h"

using namespace std;

namespace intra_epsilon_uniform {

EpsilonUniformPolicy::EpsilonUniformPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}

int EpsilonUniformPolicy::get_next_node(int partition_key){ 

    vector<Node> &partition = node_partitions.at(partition_key);
    int remove_i = -1;
    if (rng->random() > epsilon) {
        int best = numeric_limits<int>::max();
        for (int i=0; i<partition.size(); i++) { // get minimum eval index
            if (partition[i].eval < best) {
                best = partition[i].eval;
                remove_i = i;
            }
        }
    } else {
        remove_i = rng->random(partition.size());
    }

    auto ret = utils::swap_and_pop_from_vector(partition, remove_i);
    
    if (partition.size()==0)
        node_partitions.erase(partition_key);

    return ret.id;
}

void EpsilonUniformPolicy::notify_insert(
    int partition_key,
    int node_key,
    bool new_partition,
    int eval) 
{
    node_partitions[partition_key].push_back(Node(node_key, eval));
}

class EpsilonUniformPolicyFeature : public plugins::TypedFeature<NodePolicy, EpsilonUniformPolicy> {
public:
    EpsilonUniformPolicyFeature() : TypedFeature("epsilon_uniform") {
        document_subcategory("node_policies");
        document_title("Uniform Random node selection");
        document_synopsis(
            "Uniformly at random choose the next node from the partition");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.95",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_node_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<EpsilonUniformPolicyFeature> _plugin;
}