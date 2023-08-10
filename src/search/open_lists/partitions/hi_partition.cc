#include "hi_partition.h"

using namespace std;

namespace hi_partition {

HIPartition::HIPartition(const plugins::Options &opts)
    : PartitionSystem(opts), 
      rng(utils::parse_rng_from_options(opts)),
      inter_epsilon(opts.get<double>("inter_epsilon")),
      intra_epsilon(opts.get<double>("intra_epsilon")) {}


bool HIPartition::compare_parent_node_smaller(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states) {
    return open_states.at(heap[parent]).h < open_states.at(heap[child]).h;
}

bool HIPartition::compare_parent_node_bigger(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states) {
    return open_states.at(heap[parent]).h > open_states.at(heap[child]).h;
}

bool HIPartition::compare_parent_type_smaller(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states) {
    if (heaped_partitions.at(heap[parent]).empty()) return false;
    if (heaped_partitions.at(heap[child]).empty()) return true;
    return open_states.at(heaped_partitions.at(heap[parent])[0]).h 
        < open_states.at(heaped_partitions.at(heap[child])[0]).h;
}

bool HIPartition::compare_parent_type_bigger(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states) {
    if (heaped_partitions.at(heap[parent]).empty()) return true;
    if (heaped_partitions.at(heap[child]).empty()) return false;
    return open_states.at(heaped_partitions.at(heap[parent])[0]).h 
        > open_states.at(heaped_partitions.at(heap[child])[0]).h;
}


Key HIPartition::random_access_heap_pop(
        vector<Key> &heap, 
        int loc, 
        Comparer comp,
        const utils::HashMap<Key, OpenState> &open_states) 
{
    adjust_to_top(
        heap, 
        loc
    );
    swap(heap.front(), heap.back());

    Key to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0, comp, open_states);
    return to_return;
}

void HIPartition::adjust_to_top(vector<Key> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void HIPartition::adjust_heap_up(
        vector<Key> &heap, 
        int pos, 
        Comparer comp,
        const utils::HashMap<Key, OpenState> &open_states) 
{
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ((this->*comp)(heap, parent_pos, pos, open_states))
            break;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void HIPartition::adjust_heap_down(
        vector<Key> &heap, 
        int loc, 
        Comparer comp,
        const utils::HashMap<Key, OpenState> &open_states)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && ( (this->*comp)(heap, minimum, left_child_loc, open_states) ))
            minimum = left_child_loc;

    if(right_child_loc < heap.size() && ( (this->*comp)(heap, minimum, right_child_loc, open_states) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        swap(heap[loc], heap[minimum]);
        adjust_heap_down(heap, minimum, comp, open_states);
    }

}


Key HIPartition::insert_state(Key to_insert, const utils::HashMap<Key, OpenState> &open_states) {   

    OpenState state_to_insert = open_states.at(to_insert);
    OpenState parent_state = open_states.at(state_to_insert.parent_id.get_value());
    Key partition_key;
    if ( (state_to_insert.h < parent_state.h)) {
        partition_key = state_to_insert.id.get_value();
        heaped_partitions.emplace(partition_key, vector<Key>{state_to_insert.id.get_value()});
        adjust_heap_up(   
            partition_heap, 
            partition_heap.size()-1, 
            &HIPartition::compare_parent_type_smaller,
            open_states
        );
    } else {
        partition_key = parent_state.partition;
        heaped_partitions.at(partition_key).push_back(state_to_insert.id.get_value());
        adjust_heap_up(  
            heaped_partitions.at(partition_key), 
            heaped_partitions.at(partition_key).size()-1, 
            &HIPartition::compare_parent_node_smaller,
            open_states
        );
    }

    return partition_key;
}

Key HIPartition::select_next_partition(const utils::HashMap<Key, OpenState> &open_states) {
    if (last_partition_was_emptied) { // if the last partition was emptied, then insertion could result in it bubbling down the queue
        if (heaped_partitions.at(partition_heap[last_partition_index]).empty()) {
            
        } else {
            adjust_heap_down(
                partition_heap, 
                last_partition_index,
                &HIPartition::compare_parent_type_bigger,
                open_states
            );
        }

        last_partition_was_emptied = false;
    }

}

Key HIPartition::select_next_state_from_partition(Key partition_key, const utils::HashMap<Key, OpenState> &open_states) {
    
}

class HIPartitionFeature : public plugins::TypedFeature<PartitionSystem, HIPartition> {
public:
    HIPartitionFeature() : TypedFeature("hi_partition") {
        document_subcategory("partition_systems");
        document_title("Heuristic improvement partitioning");
        document_synopsis(
            "If the heuristic evaluation improved from parent to child,"
            "put child in its own type, otherwise put the child in its parent's type.");
        add_option<double>(
            "inter_epsilon",
            "probability of selecting a random partition",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        add_option<double>(
            "intra_epsilon",
            "probability of selecting a random node from a partition",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<HIPartitionFeature> _plugin;
}