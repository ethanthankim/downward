#include "epsilon_greedy_minh.h"

using namespace std;

namespace intra_partition_eg_minh {

IntraEpsilonGreedyMinHPolicy::IntraEpsilonGreedyMinHPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}

bool IntraEpsilonGreedyMinHPolicy::compare_parent_node_smaller(Key parent, Key child, utils::HashMap<Key, PartitionedState> & active_states) {
    return active_states.at(parent).h < active_states.at(child).h; 
}

bool IntraEpsilonGreedyMinHPolicy::compare_parent_node_bigger(Key parent, Key child, utils::HashMap<Key, PartitionedState> & active_states) {
    return active_states.at(parent).h > active_states.at(child).h; 
}

void IntraEpsilonGreedyMinHPolicy::adjust_heap_up(std::vector<Key> &heap, int pos, utils::HashMap<Key, PartitionedState> &active_states) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (compare_parent_node_smaller(heap[parent_pos], heap[pos], active_states))
            break;

        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void IntraEpsilonGreedyMinHPolicy::adjust_to_top(std::vector<Key> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }       
}

void IntraEpsilonGreedyMinHPolicy::adjust_heap_down(std::vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> &active_states) {
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && ( compare_parent_node_bigger(heap[minimum], heap[left_child_loc], active_states) ))
        minimum = left_child_loc;

    if(right_child_loc < heap.size() && ( compare_parent_node_bigger(heap[minimum], heap[right_child_loc], active_states) ))
        minimum = right_child_loc;

    if(minimum != loc) {
        swap(heap[loc], heap[minimum]);
        adjust_heap_down(heap, minimum, active_states);
    }
}

Key IntraEpsilonGreedyMinHPolicy::random_access_heap_pop(std::vector<Key> &heap, int loc, utils::HashMap<Key, PartitionedState> &active_states) {
    adjust_to_top(heap, loc);
    swap(heap.front(), heap.back());

    Key to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0, active_states);
    return to_return;
}

Key IntraEpsilonGreedyMinHPolicy::remove_next_state_from_partition(utils::HashMap<Key, PartitionedState> &active_states, std::vector<Key> &partition) { 
    
    active_states.erase(cached_last_removed);
    
    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition.size());
    }

    Key to_return = random_access_heap_pop(partition, pos, active_states);
    cached_last_removed = to_return;
    return to_return;
}

void IntraEpsilonGreedyMinHPolicy::insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states, std::vector<Key> &partition) {

    partition.push_back(inserted);
    adjust_heap_up( 
        partition,
        partition.size()-1, 
        active_states
    );
}

class IntraEpsilonGreedyMinHPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraEpsilonGreedyMinHPolicy> {
public:
    IntraEpsilonGreedyMinHPolicyFeature() : TypedFeature("intra_ep_minh") {
        document_subcategory("node_policies");
        document_title("Epsilon Greedy Min-h node selection");
        document_synopsis(
            "With probability epsilon, choose the best next node, otherwise"
            "choose a node uniformly at random.");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraEpsilonGreedyMinHPolicyFeature> _plugin;
}