#include "lwm_partition.h"

using namespace std;

namespace lwm_partition {

LWMPartition::LWMPartition(const plugins::Options &opts)
    : PartitionSystem(opts) {}


void LWMPartition::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
};

void LWMPartition::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) 
{
    cached_parent_id = parent_state.get_id();
    cached_next_state_id = state.get_id();
};

pair<bool, PartitionKey> LWMPartition::choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states) {   

    PartitionedState state_to_insert = active_states.at(cached_next_state_id.get_value() );
    PartitionedState parent_state = active_states.at(cached_parent_id.get_value() );
    int lwm = lwm_values.at(parent_state.partition);

    PartitionKey partition_key;
    bool new_type;
    if ( (state_to_insert.h < lwm)) {
        partition_key = state_to_insert.id.get_value();
        lwm_values[partition_key] = state_to_insert.h;
        new_type = true;
    } else {
        partition_key = parent_state.partition;
        new_type = false;
    }

    return make_pair(new_type, partition_key);
}


class LWMPartitionFeature : public plugins::TypedFeature<PartitionSystem, LWMPartition> {
public:
    LWMPartitionFeature() : TypedFeature("lwm_partition") {
        document_subcategory("partition_systems");
        document_title("Low water mark partitioning");
        document_synopsis(
            "If the heuristic evaluation improved from previous low water mark,"
            "put child in its own type, otherwise put the child in its parent's type.");
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<LWMPartitionFeature> _plugin;
}