#include "plateau_partition.h"

using namespace std;

namespace plateau_partition {

PlateauPartition::PlateauPartition(const plugins::Options &opts)
    : PartitionSystem(opts) {}


void PlateauPartition::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
};

void PlateauPartition::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) 
{
    cached_next_state_id = state.get_id();
};

pair<bool, PartitionKey> PlateauPartition::choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states, utils::HashMap<PartitionKey, Partition> partition_buckets) {   

    PartitionedState state_to_insert = active_states.at(cached_next_state_id.get_value() );
    PartitionKey partition_key = state_to_insert.h;
    bool new_type;

    if (partition_buckets.find(partition_key) == partition_buckets.end()) {
        new_type = true;
    } else {
        new_type = false;
    }

    return make_pair(new_type, partition_key);
}


class PlateauPartitionFeature : public plugins::TypedFeature<PartitionSystem, PlateauPartition> {
public:
    PlateauPartitionFeature() : TypedFeature("plateau_partition") {
        document_subcategory("partition_systems");
        document_title("Heuristic plateau partitioning");
        document_synopsis(
            "Nodes with the same heuristic evaluation get the same type.");
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PlateauPartitionFeature> _plugin;
}