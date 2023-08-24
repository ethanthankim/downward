#include "epsilon_greedy_depth.h"

using namespace std;

namespace inter_eg_depth_partition {

InterEpsilonGreedyDepthPolicy::InterEpsilonGreedyDepthPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}


bool InterEpsilonGreedyDepthPolicy::compare_parent_type_smaller(int parent, int child, 
    utils::HashMap<PartitionKey, Partition> &partition_buckets)
{
    // something is broken with the order i do things -- somehow partitions are removed from buckets but not from  the heap
    if (partition_buckets.at(partition_heap[parent]).empty()) return false;
    if (partition_buckets.at(partition_heap[child]).empty()) return true;
    return depths[partition_heap[parent]] < depths[partition_heap[child]];
}

bool InterEpsilonGreedyDepthPolicy::compare_parent_type_bigger(int parent, int child, 
    utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    if (partition_buckets.at(partition_heap[parent]).empty()) return true;
    if (partition_buckets.at(partition_heap[child]).empty()) return false;
    return depths[partition_heap[parent]] > depths[partition_heap[child]];
}

PartitionKey InterEpsilonGreedyDepthPolicy::random_access_heap_pop(
        int loc, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    adjust_to_top(loc);
    swap(partition_heap.front(), partition_heap.back());

    PartitionKey to_return = partition_heap.back();
    partition_heap.pop_back();

    adjust_heap_down(0, partition_buckets);
    return to_return;
}

void InterEpsilonGreedyDepthPolicy::adjust_to_top(int pos) {
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (cached_partition_position == pos) cached_partition_position = parent_pos;
        pos = parent_pos;
    }
}

void InterEpsilonGreedyDepthPolicy::adjust_heap_up(
        int pos, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets) 
{
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (compare_parent_type_bigger(parent_pos, pos, partition_buckets))
            break;

        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (parent_pos == cached_partition_position) cached_partition_position = pos;
        pos = parent_pos;

    }
}

void InterEpsilonGreedyDepthPolicy::adjust_heap_down(
        int loc, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < partition_heap.size() && ( compare_parent_type_smaller(minimum, left_child_loc, partition_buckets) ))
            minimum = left_child_loc;

    if(right_child_loc < partition_heap.size() && ( compare_parent_type_smaller(minimum, right_child_loc, partition_buckets) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        swap(partition_heap[loc], partition_heap[minimum]);
        if (cached_partition_position == minimum) cached_partition_position = loc;
        adjust_heap_down(minimum, partition_buckets);
    }

}


PartitionKey InterEpsilonGreedyDepthPolicy::get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) { 
// logic error in how cached variables are set and used -- follow them through...
    if ( (partition_buckets.find(cached_parent_type) != partition_buckets.end()) 
        && partition_buckets.at(cached_parent_type).empty()) {
        
        random_access_heap_pop( 
            cached_partition_position,
            partition_buckets
        );
        partition_buckets.erase(cached_parent_type);
    } else {
        adjust_heap_down(
            cached_partition_position,
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

void InterEpsilonGreedyDepthPolicy::notify_insert(bool new_type, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) {
    
    if (new_type) {
        partition_heap.push_back(active_states.at(inserted).partition);
        depths.emplace(
            active_states.at(inserted).partition, 
            depths.at(cached_parent_type) + 1    
        );

        adjust_heap_up( 
            partition_heap.size()-1, 
            partition_buckets
        );
    }
}

class InterEpsilonGreedyDepthPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterEpsilonGreedyDepthPolicy> {
public:
    InterEpsilonGreedyDepthPolicyFeature() : TypedFeature("inter_ep_depth") {
        document_subcategory("partition_policies");
        document_title("Epsilon Greedy Depth partition selection");
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

static plugins::FeaturePlugin<InterEpsilonGreedyDepthPolicyFeature> _plugin;
}