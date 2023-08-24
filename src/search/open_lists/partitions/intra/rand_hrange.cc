#include "rand_hrange.h"

using namespace std;

namespace intra_partition_eg_hrange {

IntraEpsilonGreedyHRangePolicy::IntraEpsilonGreedyHRangePolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}

bool IntraEpsilonGreedyHRangePolicy::compare_parent_node_smaller(NodeKey parent, NodeKey child) {
    return rand_hs.at(parent) < rand_hs.at(child); 
}

bool IntraEpsilonGreedyHRangePolicy::compare_parent_node_bigger(NodeKey parent, NodeKey child) {
    return rand_hs.at(parent) > rand_hs.at(child); 
}

void IntraEpsilonGreedyHRangePolicy::adjust_heap_up(std::vector<NodeKey> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (compare_parent_node_smaller(heap[parent_pos], heap[pos]))
            break;

        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void IntraEpsilonGreedyHRangePolicy::adjust_to_top(std::vector<NodeKey> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }       
}

void IntraEpsilonGreedyHRangePolicy::adjust_heap_down(std::vector<NodeKey> &heap, int loc) {
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && ( compare_parent_node_bigger(heap[minimum], heap[left_child_loc]) ))
        minimum = left_child_loc;

    if(right_child_loc < heap.size() && ( compare_parent_node_bigger(heap[minimum], heap[right_child_loc]) ))
        minimum = right_child_loc;

    if(minimum != loc) {
        swap(heap[loc], heap[minimum]);
        adjust_heap_down(heap, minimum);
    }
}

NodeKey IntraEpsilonGreedyHRangePolicy::random_access_heap_pop(std::vector<NodeKey> &heap, int loc) {
    adjust_to_top(heap, loc);
    swap(heap.front(), heap.back());

    NodeKey to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0);
    return to_return;
}

NodeKey IntraEpsilonGreedyHRangePolicy::remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) { 
    
    active_states.erase(cached_last_removed);
    
    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition.size());
    }

    NodeKey to_return = random_access_heap_pop(partition, pos);
    cached_last_removed = to_return;
    return to_return;
}

void IntraEpsilonGreedyHRangePolicy::insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) {

    partition.push_back(inserted);
    rand_hs.emplace(inserted, 
        active_states.at(inserted).h > 0 ? rng->random(active_states.at(inserted).h) : 0
    );
    adjust_heap_up( 
        partition,
        partition.size()-1
    );
}

class IntraEpsilonGreedyHRangePolicyFeature : public plugins::TypedFeature<NodePolicy, IntraEpsilonGreedyHRangePolicy> {
public:
    IntraEpsilonGreedyHRangePolicyFeature() : TypedFeature("intra_hrange") {
        document_subcategory("node_policies");
        document_title("Epsilon Greedy H-range node selection");
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

static plugins::FeaturePlugin<IntraEpsilonGreedyHRangePolicyFeature> _plugin;
}