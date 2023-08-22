#include "custom_evaluator.h"

using namespace std;

namespace intra_partition_custom_evaluator {

IntraCustomEvaluatorPolicy::IntraCustomEvaluatorPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    epsilon(opts.get<double>("epsilon")) {}

bool IntraCustomEvaluatorPolicy::compare_parent_node_smaller(NodeKey parent, NodeKey child) {
    return h_values.at(parent) < h_values.at(child); 
}

bool IntraCustomEvaluatorPolicy::compare_parent_node_bigger(NodeKey parent, NodeKey child) {
    return h_values.at(parent) > h_values.at(child); 
}

NodeKey IntraCustomEvaluatorPolicy::random_access_heap_pop(std::vector<NodeKey> &heap, int loc) {
    adjust_to_top(heap, loc);
    swap(heap.front(), heap.back());

    NodeKey to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0);
    return to_return;
}

void IntraCustomEvaluatorPolicy::adjust_heap_down(std::vector<NodeKey> &heap, int loc) {

}

void IntraCustomEvaluatorPolicy::adjust_heap_up(std::vector<NodeKey> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (compare_parent_node_smaller(heap[parent_pos], heap[pos]))
            break;

        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

void IntraCustomEvaluatorPolicy::adjust_to_top(std::vector<NodeKey> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }       
}

NodeKey IntraCustomEvaluatorPolicy::remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) { 
    active_states.erase(cached_last_removed);
    
    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition.size());
    }

    NodeKey to_return = random_access_heap_pop(partition, pos);
    cached_last_removed = to_return;
    return to_return;
}

void IntraCustomEvaluatorPolicy::insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) {
    int new_h = context.get_evaluator_value_or_infinity(evaluator.get());
    h_values[inserted] = new_h;

    partition.push_back(inserted);
    adjust_heap_up( 
        partition,
        partition.size()-1
    );
}

void IntraCustomEvaluatorPolicy::get_path_dependent_evaluators(set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

class IntraCustomEvaluatorPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraCustomEvaluatorPolicy> {
public:
    IntraCustomEvaluatorPolicyFeature() : TypedFeature("intra_custom_evaluator") {
        document_subcategory("node_policies");
        document_title("Custom evaluator node selection");
        document_synopsis(
            "Choose the next node from the partition according to a dedicated evaluator");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraCustomEvaluatorPolicyFeature> _plugin;
}