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

    utils::HashMap<int, int> lwm_values;

    StateID cached_next_state_id = StateID::no_state;
    StateID cached_parent_id = StateID::no_state;
    int last_removed = StateID::no_state.get_value();

    int parent_lwm;
    int parent_partition_key = -1;

    struct CachedInfo {
        int id;
        int eval;
        bool new_type;
        Entry entry;
        CachedInfo(const int id, int eval, bool new_type, const Entry& entry)
            : id(id), eval(eval), new_type(new_type), entry(entry) {
        }
    };
    vector<CachedInfo> successors;
    utils::HashMap<int, int> h_to_type;
    int type_counter;

protected:
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
    cached_next_state_id = initial_state.get_id();
    parent_lwm = numeric_limits<int>::max();
}

template<class Entry>
void PartitionLWMBOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    cached_next_state_id = state.get_id();
    cached_parent_id = parent_state.get_id();

    parent_partition_key = this->partitioned_nodes.at(this->cached_parent_id.get_value()).first.partition;
    parent_lwm = lwm_values[parent_partition_key];
}

template<class Entry>
void PartitionLWMBOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    if ( (new_h < parent_lwm) ) {
        bool is_new = false;
        if (h_to_type.count(new_h) == 0) {
            lwm_values[type_counter] = new_h;
            h_to_type.emplace(new_h, type_counter++);
            is_new = true;
        }
        successors.push_back(
            CachedInfo(
                this->cached_next_state_id.get_value(),
                new_h,
                is_new,
                entry
            )
        );
    } else {
        successors.push_back(
            CachedInfo(
                this->cached_next_state_id.get_value(),
                new_h,
                false,
                entry
            )
        );
        h_to_type.emplace(new_h, parent_partition_key);
    }

}

template<class Entry>
Entry PartitionLWMBOpenList<Entry>::remove_min() {

    for(CachedInfo info : successors) {
        int part_key = h_to_type.at(info.eval);
        this->partition_selector->notify_partition_transition(
            parent_partition_key, 
            cached_parent_id.get_value(), 
            part_key, 
            info.id);
        PartitionOpenList<Entry>::partition_insert(info.id, info.eval, info.entry, part_key, info.new_type);
    }
    successors.clear();
    h_to_type.clear();

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
    return this->partitioned_nodes.empty() && successors.empty();
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