#include "inter_epsilon_greedy_minh.h"

using namespace std;

namespace inter_eg_minh_partition {

InterEpsilonGreedyMinHPolicy::InterEpsilonGreedyMinHPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}



inline int InterEpsilonGreedyMinHPolicy::state_h(int state_id, utils::HashMap<Key, PartitionedState> active_states) {
    return active_states.at(state_id).h;
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_node_smaller(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states) {
    return state_h(heap[parent], active_states) < state_h(heap[child], active_states);
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_node_bigger(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states) {
    return state_h(heap[parent], active_states) > state_h(heap[child], active_states);
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_type_smaller(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states) {
    if (partition_buckets.at(heap[parent]).second.empty()) return false;
    if (partition_buckets.at(heap[child]).second.empty()) return true;
    return state_h(partition_buckets.at(heap[parent]).second[0], active_states) < state_h(partition_buckets.at(heap[child]).second[0], active_states);
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_type_bigger(vector<Key>& heap, int parent, int child, utils::HashMap<Key, PartitionedState> active_states) {
    if (partition_buckets.at(heap[parent]).second.empty()) return true;
    if (partition_buckets.at(heap[child]).second.empty()) return false;
    return state_h(partition_buckets.at(heap[parent]).second[0], active_states) > state_h(partition_buckets.at(heap[child]).second[0], active_states);
}

void InterEpsilonGreedyMinHPolicy::sync_type_heap_and_type_location(vector<Key> &heap, int pos1, int pos2) {
    partition_buckets.at(heap[pos2]).first = pos1;
    partition_buckets.at(heap[pos1]).first = pos2;
}

Key InterEpsilonGreedyMinHPolicy::random_access_heap_pop(
        vector<Key> &heap, 
        int loc, 
        utils::HashMap<Key, PartitionedState> active_states,
        Comparer comp, 
        Synchronizer sync) 
{
    adjust_to_top(
        heap, 
        loc,
        active_states,
        sync
    );
    swap(heap.front(), heap.back());
    if (sync) (this->*sync)(heap, 0, 0);

    Key to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0, active_states, comp, sync);
    return to_return;
}

void InterEpsilonGreedyMinHPolicy::adjust_to_top(vector<Key> &heap, int pos, utils::HashMap<Key, PartitionedState> active_states, Synchronizer sync) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (sync) (this->*sync)(heap, pos, parent_pos);
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void InterEpsilonGreedyMinHPolicy::adjust_heap_up(
        vector<Key> &heap, 
        int pos, 
        utils::HashMap<Key, PartitionedState> active_states,
        Comparer comp,
        Synchronizer sync) 
{
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ((this->*comp)(heap, parent_pos, pos, active_states))
            break;
        if (sync) (this->*sync)(heap, parent_pos, pos);
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void InterEpsilonGreedyMinHPolicy::adjust_heap_down(
        vector<Key> &heap, 
        int loc, 
        utils::HashMap<Key, PartitionedState> active_states,
        Comparer comp,
        Synchronizer sync)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && ( (this->*comp)(heap, minimum, left_child_loc, active_states) ))
            minimum = left_child_loc;

    if(right_child_loc < heap.size() && ( (this->*comp)(heap, minimum, right_child_loc, active_states) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        if (sync) (this->*sync)(heap, loc, minimum);
        swap(heap[loc], heap[minimum]);
        adjust_heap_down(heap, minimum, active_states, comp, sync);
    }

}



Key InterEpsilonGreedyMinHPolicy::remove_min() { 

    // if ( (type_buckets.find(cached_parent_key) != type_buckets.end()) 
    //     && type_buckets.at(cached_parent_key).second.empty()) {
        
    //     Key erase_key = random_access_heap_pop(
    //         type_heap, 
    //         type_buckets.at(cached_parent_key).first,
    //         &BTSInterGreedyIntraEpOpenList<Entry>::compare_parent_type_bigger,
    //         &BTSInterGreedyIntraEpOpenList<Entry>::sync_type_heap_and_type_location
    //     );
    //     type_buckets.erase(cached_parent_key);
    // }

    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition_heap.size());
    }

    return partition_heap[pos];
}

void InterEpsilonGreedyMinHPolicy::notify_insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states) {

    PartitionedState inserted_state = active_states.at(inserted);
    Key partition_to_insert_into = inserted_state.partition;

    partition_buckets.at(partition_to_insert_into).second.push_back(inserted);
    adjust_heap_up(
        partition_buckets.at(partition_to_insert_into).second,
        partition_buckets.at(partition_to_insert_into).second.size()-1,
        active_states,
        &InterEpsilonGreedyMinHPolicy::compare_parent_node_smaller
    );

    adjust_heap_up(
        partition_heap, 
        partition_buckets.at(partition_to_insert_into).first, 
        active_states,
        &InterEpsilonGreedyMinHPolicy::compare_parent_type_smaller,
        &InterEpsilonGreedyMinHPolicy::sync_type_heap_and_type_location
    );
}

void InterEpsilonGreedyMinHPolicy::notify_remove(Key removed, utils::HashMap<Key, PartitionedState> active_states) {
    
}


class InterEpsilonGreedyMinHPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterEpsilonGreedyMinHPolicy> {
public:
    InterEpsilonGreedyMinHPolicyFeature() : TypedFeature("inter_ep_minh") {
        document_subcategory("partition_policies");
        document_title("Epsilon Greedy Min-h partition selection");
        document_synopsis(
            "With probability epsilon, choose the best next partition, otherwise"
            "choose a paritition uniformly at random.");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterEpsilonGreedyMinHPolicyFeature> _plugin;
}