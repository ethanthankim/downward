#include "epsilon_greedy_minh.h"

using namespace std;

namespace inter_eg_minh_partition {

InterEpsilonGreedyMinHPolicy::InterEpsilonGreedyMinHPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}



inline int InterEpsilonGreedyMinHPolicy::state_h(int state_id, utils::HashMap<NodeKey, PartitionedState> &active_states) {
    return active_states.at(state_id).h;
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_type_smaller(int parent, int child, 
    utils::HashMap<NodeKey, PartitionedState> &active_states,
    utils::HashMap<PartitionKey, Partition> &partition_buckets)
{
    // something is broken with the order i do things -- somehow partitions are removed from buckets but not from  the heap
    if (partition_buckets.at(partition_heap[parent]).empty()) return false;
    if (partition_buckets.at(partition_heap[child]).empty()) return true;
    return state_h(partition_buckets.at(partition_heap[parent])[0], active_states) < state_h(partition_buckets.at(partition_heap[child])[0], active_states);
}

bool InterEpsilonGreedyMinHPolicy::compare_parent_type_bigger(int parent, int child, 
    utils::HashMap<NodeKey, PartitionedState> &active_states,
    utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    if (partition_buckets.at(partition_heap[parent]).empty()) return true;
    if (partition_buckets.at(partition_heap[child]).empty()) return false;
    return state_h(partition_buckets.at(partition_heap[parent])[0], active_states) > state_h(partition_buckets.at(partition_heap[child])[0], active_states);
}

PartitionKey InterEpsilonGreedyMinHPolicy::random_access_heap_pop(
        int loc, 
        utils::HashMap<NodeKey, PartitionedState> &active_states,
        utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    adjust_to_top(loc);
    swap(partition_heap.front(), partition_heap.back());

    PartitionKey to_return = partition_heap.back();
    partition_heap.pop_back();

    adjust_heap_down(0, active_states, partition_buckets);
    return to_return;
}

void InterEpsilonGreedyMinHPolicy::adjust_to_top(int pos) {
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (cached_partition_position == pos) cached_partition_position = parent_pos;
        pos = parent_pos;
    }
}

void InterEpsilonGreedyMinHPolicy::adjust_heap_up(
        int pos, 
        utils::HashMap<NodeKey, PartitionedState> &active_states,
        utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (compare_parent_type_smaller(parent_pos, pos, active_states, partition_buckets))
            break;

        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (parent_pos == cached_partition_position) cached_partition_position = pos;
        pos = parent_pos;

    }
}

void InterEpsilonGreedyMinHPolicy::adjust_heap_down(
        int loc, 
        utils::HashMap<NodeKey, PartitionedState> &active_states,
        utils::HashMap<PartitionKey, Partition> &partition_buckets)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < partition_heap.size() && ( compare_parent_type_bigger(minimum, left_child_loc, active_states, partition_buckets) ))
            minimum = left_child_loc;

    if(right_child_loc < partition_heap.size() && ( compare_parent_type_bigger(minimum, right_child_loc, active_states, partition_buckets) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        swap(partition_heap[loc], partition_heap[minimum]);
        if (cached_partition_position == minimum) cached_partition_position = loc;
        adjust_heap_down(minimum, active_states, partition_buckets);
    }

}


PartitionKey InterEpsilonGreedyMinHPolicy::get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) { 
// logic error in how cached variables are set and used -- follow them through...
    if ( (partition_buckets.find(cached_parent_type) != partition_buckets.end()) 
        && partition_buckets.at(cached_parent_type).empty()) {
        
        random_access_heap_pop( 
            cached_partition_position,
            active_states,
            partition_buckets
        );
        partition_buckets.erase(cached_parent_type);
    } else {
        adjust_heap_down(
            cached_partition_position,
            active_states,
            partition_buckets
        );
    }

    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition_heap.size());
    }

    cached_parent_type = partition_heap[pos];
    cached_partition_position = pos;
    return partition_heap[pos];
}

void InterEpsilonGreedyMinHPolicy::notify_insert(bool new_type, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) {
    // TODO: create a robust way to check if a type is new or not. Maybe just a flag.
    if (new_type) {
        partition_heap.push_back(active_states.at(inserted).partition);

        adjust_heap_up( 
            partition_heap.size()-1, 
            active_states,
            partition_buckets
        );
    }
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