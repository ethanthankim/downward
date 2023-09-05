#include "epsilon_greedy_root.h"

using namespace std;

namespace inter_eg_root_partition {

InterEpsilonGreedyRootPolicy::InterEpsilonGreedyRootPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    epsilon(opts.get<double>("epsilon")) {}

int InterEpsilonGreedyRootPolicy::remove_last_chosen_partition() 
{
    int loc = last_chosen_partition_index;
    int tmp_part = partition_heap.back().partition;
    swap(partition_heap[loc], partition_heap.back());

    auto to_return = partition_heap.back();
    partition_heap.pop_back();

    last_chosen_partition = -1;
    last_chosen_partition_index = -1;
    if (loc < partition_heap.size()){
        adjust_heap(loc);
    }
    return to_return.partition;
}

void InterEpsilonGreedyRootPolicy::adjust_heap(int pos) {
    assert(utils::in_bounds(pos, partition_heap));
    
    if (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ( partition_heap[parent_pos] > partition_heap[pos] ) {
            adjust_heap_up(pos);
            return;
        }
    }
    adjust_heap_down(pos);
}

void InterEpsilonGreedyRootPolicy::adjust_heap_up(int pos) 
{
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ( partition_heap[parent_pos] < partition_heap[pos] ) break;

        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (last_chosen_partition_index == pos) last_chosen_partition_index = parent_pos;
        else if (last_chosen_partition_index == parent_pos) last_chosen_partition_index = pos;
        pos = parent_pos;

    }
}

void InterEpsilonGreedyRootPolicy::adjust_heap_down(int loc)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < partition_heap.size() && partition_heap[left_child_loc] < partition_heap[minimum])
        minimum = left_child_loc;

    if(right_child_loc < partition_heap.size() && partition_heap[right_child_loc] < partition_heap[minimum]) // TODO: COMPARE
        minimum = right_child_loc;

    if(minimum != loc) {
        swap(partition_heap[loc], partition_heap[minimum]);
        if (last_chosen_partition_index == loc) last_chosen_partition_index = minimum;
        else if (last_chosen_partition_index == minimum) last_chosen_partition_index = loc;
        adjust_heap_down(minimum);
    }

}


int InterEpsilonGreedyRootPolicy::get_next_partition() { 

    // might need to remove newly emptied partition
    if (last_chosen_partition_index != -1) { 
        PartitionNode &removed_from = partition_heap[last_chosen_partition_index];
        if (partition_heap[last_chosen_partition_index].size == 0) {
            remove_last_chosen_partition();
        }
    }

    int pos = 0;
    if (rng->random() < epsilon) {
        pos = rng->random(partition_heap.size());
    }
    PartitionNode &part_node = partition_heap[pos];

    last_chosen_partition_index=pos;
    last_chosen_partition = part_node.partition;
    return part_node.partition;

}

void InterEpsilonGreedyRootPolicy::notify_insert(
    int partition_key,
    int node_key,
    bool new_partition,
    EvaluationContext &eval_context) 
{
    // handle new partitioned node
    int eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int pos;

    if (new_partition) {
        partition_heap.push_back(PartitionNode(
            partition_key, 
            1,
            eval
        ));
        pos = partition_heap.size()-1;
    } else {
        PartitionNode &inserted = partition_heap[last_chosen_partition_index];
        inserted.size += 1;
        pos = last_chosen_partition_index;
    }
    adjust_heap(pos);

}

void InterEpsilonGreedyRootPolicy::notify_removal(int partition_key, int node_key) {
    partition_heap[last_chosen_partition_index].size -= 1;
}

class InterEpsilonGreedyRootPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterEpsilonGreedyRootPolicy> {
public:
    InterEpsilonGreedyRootPolicyFeature() : TypedFeature("inter_ep_root") {
        document_subcategory("partition_policies");
        document_title("Epsilon Greedy Root partition selection");
        document_synopsis(
            "With probability epsilon, choose the best next partition, otherwise"
            "choose a paritition uniformly at random.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterEpsilonGreedyRootPolicyFeature> _plugin;
}