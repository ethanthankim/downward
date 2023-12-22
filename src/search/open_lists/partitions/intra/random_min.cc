#include "random_min.h"

#define RANDOM_BOUND 1000


using namespace std;

namespace intra_partition_random_min {

IntraRandomMinPolicy::IntraRandomMinPolicy(const plugins::Options &opts)
    : NodePolicy(opts),
    rng(utils::parse_rng_from_options(opts)) {}

int IntraRandomMinPolicy::get_next_node(int partition_key) {
    auto &heap = partition_heaps.at(partition_key);
    pop_heap(heap.begin(), heap.end(), greater<HeapNode>());
    HeapNode heap_node = heap.back();
    heap.pop_back();
    return heap_node.id;
}

void IntraRandomMinPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) 
{
    if (new_partition) {
        partition_heaps.emplace(partition_key, vector<HeapNode>());
    }

    auto &heap = partition_heaps.at(partition_key);
    heap.push_back( HeapNode(node_key, rng->random(RANDOM_BOUND)) ); // random eval
    push_heap(heap.begin(), heap.end(), greater<HeapNode>());
}

class IntraRandomMinPolicyFeature : public plugins::TypedFeature<NodePolicy, IntraRandomMinPolicy> {
public:
    IntraRandomMinPolicyFeature() : TypedFeature("random_min") {
        document_subcategory("node_policies");
        document_title("Random Min node selection");
        document_synopsis(
            "Random node eval min heap");
        utils::add_rng_options(*this);
        add_node_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<IntraRandomMinPolicyFeature> _plugin;
}