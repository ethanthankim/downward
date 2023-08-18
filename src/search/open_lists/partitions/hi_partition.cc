#include "hi_partition.h"

using namespace std;

namespace hi_partition {

HIPartition::HIPartition(const plugins::Options &opts)
    : PartitionSystem(opts) {}


void HIPartition::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
};

void HIPartition::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state) 
{
    cached_parent_id = parent_state.get_id();
    cached_next_state_id = state.get_id();
};

Key HIPartition::choose_state_partition(utils::HashMap<Key, PartitionedState> active_states) {   

    PartitionedState state_to_insert = active_states.at(cached_next_state_id.get_value() );
    PartitionedState parent_state = active_states.at(cached_parent_id.get_value());
    Key partition_key;
    if ( (state_to_insert.h < parent_state.h)) {
        partition_key = state_to_insert.id.get_value();
    } else {
        partition_key = parent_state.partition;
    }

    return partition_key;
}


class HIPartitionFeature : public plugins::TypedFeature<PartitionSystem, HIPartition> {
public:
    HIPartitionFeature() : TypedFeature("hi_partition") {
        document_subcategory("partition_systems");
        document_title("Heuristic improvement partitioning");
        document_synopsis(
            "If the heuristic evaluation improved from parent to child,"
            "put child in its own type, otherwise put the child in its parent's type.");
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<HIPartitionFeature> _plugin;
}