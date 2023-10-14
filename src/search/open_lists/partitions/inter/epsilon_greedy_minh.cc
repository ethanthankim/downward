#include "epsilon_greedy_minh.h"

using namespace std;

namespace inter_eg_minh_partition {

InterEpsilonGreedyMinHPolicy::InterEpsilonGreedyMinHPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    epsilon(opts.get<double>("epsilon")) {}

int InterEpsilonGreedyMinHPolicy::remove_last_chosen_partition() 
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

void InterEpsilonGreedyMinHPolicy::adjust_heap(int pos) {
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

void InterEpsilonGreedyMinHPolicy::adjust_heap_up(int pos) 
{
    assert(utils::in_bounds(pos, partition_heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ( partition_heap[parent_pos] <= partition_heap[pos] ) break;

        swap(partition_heap[pos], partition_heap[parent_pos]);
        if (last_chosen_partition_index == pos) last_chosen_partition_index = parent_pos;
        else if (last_chosen_partition_index == parent_pos) last_chosen_partition_index = pos;
        pos = parent_pos;

    }
}

void InterEpsilonGreedyMinHPolicy::adjust_heap_down(int loc)
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


int InterEpsilonGreedyMinHPolicy::get_next_partition() { 

    // might need to remove newly emptied partition
    if (last_chosen_partition_index != -1) { 
        PartitionNode &removed_from = partition_heap[last_chosen_partition_index];
        if (removed_from.state_hs.at(removed_node_h) == 0) {
            removed_from.state_hs.erase(removed_node_h);
            if (partition_heap.at(last_chosen_partition_index).state_hs.empty()) {
                remove_last_chosen_partition();
            } else {
                adjust_heap(last_chosen_partition_index);
            }
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

void InterEpsilonGreedyMinHPolicy::notify_insert(
    int partition_key,
    int node_key,
    bool new_partition,
    EvaluationContext &eval_context) 
{
    // handle new partitioned node
    int eval = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    node_hs.emplace(node_key, eval);
    int pos;

    if (new_partition) {
        partition_heap.push_back(PartitionNode(
            partition_key, 
            map<int, int>{make_pair(eval, 1)}
        ));
        pos = partition_heap.size()-1;
        // log << "[notify_insert] NEW PARTITION. id: " << partition_key << " , eval: " << eval << endl; 
    } else {
        PartitionNode &inserted = partition_heap[last_chosen_partition_index];
        inserted.state_hs[eval] += 1;
        pos = last_chosen_partition_index;
    }
    adjust_heap(pos);

    // log << "[notify_insert] partition id: " << partition_key  
    //     << " , node id: " << node_key
    //     << " , h: " << eval << endl;

}

void InterEpsilonGreedyMinHPolicy::notify_removal(int partition_key, int node_key) {
    removed_node_h = node_hs.at(node_key);
    partition_heap[last_chosen_partition_index].state_hs.at(removed_node_h) -= 1;
    node_hs.erase(node_key);
}

class InterEpsilonGreedyMinHPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterEpsilonGreedyMinHPolicy> {
public:
    InterEpsilonGreedyMinHPolicyFeature() : TypedFeature("inter_ep_minh") {
        document_subcategory("partition_policies");
        document_title("Epsilon Greedy Min-h partition selection");
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

static plugins::FeaturePlugin<InterEpsilonGreedyMinHPolicyFeature> _plugin;
}