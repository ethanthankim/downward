#include "random_partition.h"

using namespace std;

namespace random_partition {

RandomPartition::RandomPartition(const plugins::Options &opts)
    : PartitionSystem(opts), 
      rng(utils::parse_rng_from_options(opts)),
      partition_system(opts.get<shared_ptr<PartitionSystem>>("partition")),
      epsilon_use_partition(opts.get<double>("epsilon_partition")),
      epsilon_can_distinguish(opts.get<double>("epsilon_new"))  {}

void RandomPartition::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
    partition_system->notify_initial_state(initial_state);
};

void RandomPartition::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) 
{
    cached_parent_id = parent_state.get_id();
    cached_next_state_id = state.get_id();
    partition_system->notify_state_transition(parent_state, op_id, state);
};

pair<bool, PartitionKey> RandomPartition::choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states, utils::HashMap<PartitionKey, Partition> partition_buckets) {   

    if (rng->random() < epsilon_use_partition) {
        return partition_system->choose_state_partition(active_states, partition_buckets);
    } else {
        PartitionedState state_to_insert = active_states.at(cached_next_state_id.get_value());
        PartitionedState parent_state = active_states.at(cached_parent_id.get_value());
        PartitionKey partition;
        bool new_type;

        if (rng->random() < epsilon_can_distinguish) {
            return make_pair(true, state_to_insert.id.get_value()); // can distinguish from other nodes
        } else {
            auto it = partition_buckets.begin();
            std::advance(it, rng->random(partition_buckets.size()));
            return make_pair(false, it->first); // cannot distinguish from other nodes
        }
    }
}

class RandomPartitionFeature : public plugins::TypedFeature<PartitionSystem, RandomPartition> {
public:
    RandomPartitionFeature() : TypedFeature("random_partition") {
        document_subcategory("partition_systems");
        document_title("random partitioning");
        document_synopsis(
            "This is primarily used for testing the added value of partition systems and selection policies");
        add_option<shared_ptr<PartitionSystem>>("partition", "underlying partition system");
        add_option<double>(
            "epsilon_partition",
            "probability of partitioning a node randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        add_option<double>(
            "epsilon_new",
            "probability of node getting a new partition",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<RandomPartitionFeature> _plugin;
}