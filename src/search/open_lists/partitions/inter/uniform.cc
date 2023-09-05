#include "uniform.h"

using namespace std;

namespace inter_uniform_partition {

InterUniformPolicy::InterUniformPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}


int InterUniformPolicy::get_next_partition() { 

    int chosen_partition;
    while(true) {
        auto it = partition_sizes.begin();
        std::advance(it, rng->random(partition_sizes.size()));
        chosen_partition = it->first;

        if (partition_sizes.at(chosen_partition) == 0){
            partition_sizes.erase(chosen_partition);
            continue;
        }

        return chosen_partition;

    } 
    
}

void InterUniformPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) 
{
    partition_sizes[partition_key] += 1;
}

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