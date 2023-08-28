#include "h_partition.h"

using namespace std;

namespace h_partition {

HPartition::HPartition(const plugins::Options &opts)
    : PartitionSystem(opts) {}


void HPartition::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
};

void HPartition::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) 
{
    cached_parent_id = parent_state.get_id();
    cached_next_state_id = state.get_id();
};

pair<bool, PartitionKey> HPartition::choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states, utils::HashMap<PartitionKey, Partition> partition_buckets) {   

    PartitionedState state_to_insert = active_states.at(cached_next_state_id.get_value() );
    PartitionedState parent_state = active_states.at(cached_parent_id.get_value());
    PartitionKey partition_key;
    bool new_type;
    if ( (state_to_insert.h != parent_state.h)) {
        partition_key = state_to_insert.id.get_value();
        new_type = true;
    } else {
        partition_key = parent_state.partition;
        new_type = false;
    }

    return make_pair(new_type, partition_key);
}


class HPartitionFeature : public plugins::TypedFeature<PartitionSystem, HPartition> {
public:
    HPartitionFeature() : TypedFeature("h_partition") {
        document_subcategory("partition_systems");
        document_title("Heuristic plateau partitioning");
        document_synopsis(
            "If the heuristic evaluation is different from parent to child,"
            "put child in its own type, otherwise put the child in its parent's type.");
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<HPartitionFeature> _plugin;
}