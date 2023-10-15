#include "partition_lwm_open_list.h"

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

namespace partition_lwm_open_list {
template<class Entry>
class PartitionLWMOpenList : public PartitionOpenList<Entry> {

    utils::HashMap<int, int> lwm_values;

    StateID cached_next_state_id = StateID::no_state;
    StateID cached_parent_id = StateID::no_state;

    int last_removed = StateID::no_state.get_value();
    int parent_lwm;
    int parent_partition_key = -1;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;
    virtual void clear() override;

public:
    Entry remove_min() override;
    explicit PartitionLWMOpenList(const plugins::Options &opts);
    virtual ~PartitionLWMOpenList() override = default;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;

};

template<class Entry>
void PartitionLWMOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
    parent_lwm = numeric_limits<int>::max();
}

template<class Entry>
void PartitionLWMOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    cached_next_state_id = state.get_id();
    cached_parent_id = parent_state.get_id();

    parent_partition_key = this->partitioned_nodes.at(this->cached_parent_id.get_value()).first.partition;
    parent_lwm = lwm_values[parent_partition_key];
}

template<class Entry>
void PartitionLWMOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int partition_key;
    bool new_type;
    if ( (new_h < parent_lwm)) {
        partition_key = this->cached_next_state_id.get_value();
        lwm_values[partition_key] = new_h;
        new_type = true;
    } else {
        partition_key = parent_partition_key;
        new_type = false;
    }

    this->partition_selector->notify_partition_transition(parent_partition_key, partition_key);

    PartitionOpenList<Entry>::partition_insert(eval_context, new_h, entry, partition_key, new_type);
}

template<class Entry>
Entry PartitionLWMOpenList<Entry>::remove_min() {

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
void PartitionLWMOpenList<Entry>::clear() {
    PartitionOpenList<Entry>::clear();
    lwm_values.clear();
}

template<class Entry>
PartitionLWMOpenList<Entry>::PartitionLWMOpenList(const plugins::Options &opts)
    : PartitionOpenList<Entry>(
        opts.get<shared_ptr<Evaluator>>("eval"),
        opts.get<shared_ptr<PartitionPolicy>>("partition_policy"),
        opts.get<shared_ptr<NodePolicy>>("node_policy")
    ) {}


PartitionLWMOpenListFactory::PartitionLWMOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionLWMOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionLWMOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionLWMOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionLWMOpenList<EdgeOpenListEntry>>(options);
}

class PartitionLWMOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionLWMOpenListFactory> {
public:
    PartitionLWMOpenListFeature() : TypedFeature("lwm_partition") {
        document_title("Partition Heuristic Improvement Open List");
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

static plugins::FeaturePlugin<PartitionLWMOpenListFeature> _plugin;
}