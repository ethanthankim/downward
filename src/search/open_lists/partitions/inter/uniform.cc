#include "uniform.h"

#include "../utils/collections.h"

using namespace std;

namespace inter_uniform_partition {

InterUniformPolicy::InterUniformPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}


int InterUniformPolicy::get_next_partition() { 

    size_t bucket_id = rng->random(partition_keys_and_sizes.size());
    pair<int, int> &part = partition_keys_and_sizes[bucket_id];
    
    return part.first;
    
}

void InterUniformPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) 
{
    auto it = key_to_partition_index.find(partition_key);
    if (it == key_to_partition_index.end()) {
        key_to_partition_index[partition_key] = partition_keys_and_sizes.size();
        partition_keys_and_sizes.push_back(make_pair(partition_key, 1));
    } else {
        int part_index = it->second;
        partition_keys_and_sizes[part_index].second += 1;
    }
}

void InterUniformPolicy::notify_removal(int partition_key, int node_key) {
    int part_index = key_to_partition_index.at(partition_key);
    pair<int, int> &part = partition_keys_and_sizes[part_index];
    part.second-=1;

    if (part.second == 0) {
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_partition_index[partition_keys_and_sizes.back().first] = part_index;
        key_to_partition_index.erase(partition_key);
        utils::swap_and_pop_from_vector(partition_keys_and_sizes, part_index);
    }
};

class InterUniformPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterUniformPolicy> {
public:
    InterUniformPolicyFeature() : TypedFeature("inter_uniform") {
        document_subcategory("partition_policies");
        document_title("Uniform Random partition selection");
        document_synopsis(
            "Choose the next partition uniformly at random.");
        utils::add_rng_options(*this);
        add_partition_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterUniformPolicyFeature> _plugin;
}