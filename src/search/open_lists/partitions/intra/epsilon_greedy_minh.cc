#include "epsilon_greedy_minh.h"

using namespace std;

namespace intra_partition_eg_minh {

template<class HeapNode>
static void adjust_heap_up(vector<HeapNode> &heap, size_t pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (heap[pos] > heap[parent_pos]) {
            break;
        }
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

IntraEpsilonGreedyMinHPolicy::IntraEpsilonGreedyMinHPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    rng(utils::parse_rng_from_options(opts)),
    epsilon(opts.get<double>("epsilon")) {}

int IntraEpsilonGreedyMinHPolicy::get_next_node(int partition_key) {
    auto &heap = partition_heaps.at(partition_key);

    if (rng->random() < epsilon) {
        int pos = rng->random(heap.size());
        heap[pos].h = numeric_limits<int>::min();
        adjust_heap_up(heap, pos);
    }
    pop_heap(heap.begin(), heap.end(), greater<HeapNode>());
    HeapNode heap_node = heap.back();
    heap.pop_back();

    // log << "[get_next_node] partition id: " << partition_key
    //     << " , node key: " << heap_node.id
    //     << " , node eval: " << log_eval << endl;
    return heap_node.id;
}

void IntraEpsilonGreedyMinHPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) 
{
    if (new_partition) {
        partition_heaps.emplace(partition_key, vector<HeapNode>());
    }

    auto &heap = partition_heaps.at(partition_key);
    heap.push_back( HeapNode(node_key, eval_context.get_evaluator_value(evaluator.get())) );   
    push_heap(heap.begin(), heap.end(), greater<HeapNode>());
}

class IntraEpsilonGreedyMinHPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraEpsilonGreedyMinHPolicy> {
public:
    IntraEpsilonGreedyMinHPolicyFeature() : TypedFeature("intra_ep_minh") {
        document_subcategory("node_policies");
        document_title("Epsilon Greedy Min-h node selection");
        document_synopsis(
            "With probability epsilon, choose the best next node, otherwise"
            "choose a node uniformly at random.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_node_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraEpsilonGreedyMinHPolicyFeature> _plugin;
}