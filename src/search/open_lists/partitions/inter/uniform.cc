#include "uniform.h"

using namespace std;

namespace inter_uniform_partition {

InterUniformPolicy::InterUniformPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}


PartitionKey InterUniformPolicy::get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) { 

    if ( (partition_buckets.find(cached_parent_type) != partition_buckets.end()) 
        && partition_buckets.at(cached_parent_type).empty()) 
    {
        partition_buckets.erase(cached_parent_type);
    } 

    auto it = partition_buckets.begin();
    advance(it, rng->random(partition_buckets.size()));
    PartitionKey cached_parent_type = it->first;

    return cached_parent_type;
}

class InterUniformPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterUniformPolicy> {
public:
    InterUniformPolicyFeature() : TypedFeature("inter_uniform") {
        document_subcategory("partition_policies");
        document_title("Uniform Random partition selection");
        document_synopsis(
            "Choose the next partition uniformly at random.");
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterUniformPolicyFeature> _plugin;
}