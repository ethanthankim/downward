#include "partition_lwmb_open_list.h"

#include "partitions/partition_open_list.h"
#include "partitions/inter/partition_policy.h"
#include "partitions/intra/node_policy.h"
#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace partition_open_list;

namespace partition_lwmb_open_list {
template<class Entry>
class PartitionLWMBOpenList : public PartitionOpenList<Entry> {

    bool start_new_expansion = true;
    StateID curr_expanding = StateID::no_state;
    StateID child_of_curr_expanding = StateID::no_state;
    int last_removed = StateID::no_state.get_value();
    int curr_expanding_lwm;
    int curr_expanding_part_key = -1;

    utils::HashMap<int, int> lwm_values;
    utils::HashMap<int, int> h_to_type;
    int type_counter;

protected:
    // virtual bool new_expansion();
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

    virtual void clear() override;

public:
    Entry remove_min() override;
    explicit PartitionLWMBOpenList(const plugins::Options &opts);
    virtual ~PartitionLWMBOpenList() override = default;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;

    virtual bool empty() const override;


};

template<class Entry>
void PartitionLWMBOpenList<Entry>::notify_initial_state(const State &initial_state) {
    child_of_curr_expanding = initial_state.get_id();
    curr_expanding_lwm = numeric_limits<int>::max();
}

template<class Entry>
void PartitionLWMBOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    if (parent_state.get_id() != curr_expanding) {
        start_new_expansion = true;
        curr_expanding = parent_state.get_id();
    } else {
        start_new_expansion = false;
    }

    child_of_curr_expanding = state.get_id();
    curr_expanding_part_key = this->partitioned_nodes.at(curr_expanding.get_value()).first.partition;
    curr_expanding_lwm = lwm_values[curr_expanding_part_key];
}

// template<class Entry>
// bool PartitionLWMBOpenList<Entry>::new_expansion() {
//     if (start_new_expansion) {
//         start_new_expansion = false;
//         return true;
//     }
//     return start_new_expansion;
// }

template<class Entry>
void PartitionLWMBOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    if (start_new_expansion) {
        h_to_type.clear();
    }

    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    bool is_new_part = false;
    int partition_key;
    if ( (new_h < curr_expanding_lwm) ) {
        if (h_to_type.count(new_h) == 0) {
            lwm_values[type_counter] = new_h;
            h_to_type.emplace(new_h, type_counter);
            partition_key = type_counter++;
            is_new_part = true;
        } else {
            partition_key = h_to_type.at(new_h);
        }
    } else {
        h_to_type.emplace(new_h, curr_expanding_part_key);
        partition_key = curr_expanding_part_key;
    }

    this->partition_selector->notify_partition_transition(
        curr_expanding_part_key, 
        curr_expanding.get_value(), 
        partition_key, 
        child_of_curr_expanding.get_value());

    PartitionOpenList<Entry>::partition_insert(child_of_curr_expanding.get_value(), new_h, entry, partition_key, is_new_part);


}

template<class Entry>
Entry PartitionLWMBOpenList<Entry>::remove_min() {

    if (last_removed != StateID::no_state.get_value()) {
        this->partition_selector->notify_removal(this->partitioned_nodes.at(last_removed).first.partition, last_removed);
        this->partitioned_nodes.erase(last_removed);
    }

    int chosen_partition = this->partition_selector->get_next_partition();
    int chosen_node = this->node_selector->get_next_node(chosen_partition);

    Entry result = this->partitioned_nodes.at(chosen_node).second;
    last_removed = chosen_node;
    return result;

}

template<class Entry>
bool PartitionLWMBOpenList<Entry>::empty() const {
    return this->partitioned_nodes.empty();
}

template<class Entry>
void PartitionLWMBOpenList<Entry>::clear() {
    PartitionOpenList<Entry>::clear();
    lwm_values.clear();
}


template<class Entry>
PartitionLWMBOpenList<Entry>::PartitionLWMBOpenList(const plugins::Options &opts)
    : PartitionOpenList<Entry>(
        opts.get<shared_ptr<Evaluator>>("eval"),
        opts.get<shared_ptr<PartitionPolicy>>("partition_policy"),
        opts.get<shared_ptr<NodePolicy>>("node_policy")
    ),
    type_counter(0) {}


PartitionLWMBOpenListFactory::PartitionLWMBOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionLWMBOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionLWMBOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionLWMBOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionLWMBOpenList<EdgeOpenListEntry>>(options);
}

class PartitionLWMBOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionLWMBOpenListFactory> {
public:
    PartitionLWMBOpenListFeature() : TypedFeature("lwmb_partition") {
        document_title("Partition Heuristic Improvement Bench Open List");
        document_synopsis("A configurable open list that selects nodes by first"
         "choosing a node parition and then choosing a node from within it."
         "The policies for insertion (choosing a partition to insert into or"
         "creating a new one) and selection (partition first, then a node within it)"
         "are  specified policies");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<shared_ptr<PartitionPolicy>>("partition_policy", "partition selection policy");
        add_option<shared_ptr<NodePolicy>>("node_policy", "node selection policy");
    }
};

static plugins::FeaturePlugin<PartitionLWMBOpenListFeature> _plugin;
}