#include "partition_open_list.h"

#include "partitions/partition_system.h"
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


namespace partition_open_list {
template<class Entry>
class PartitionOpenList : public OpenList<Entry> {
    shared_ptr<PartitionSystem> partition_system;
    shared_ptr<Evaluator> evaluator;

    utils::HashMap<Key, OpenState> active_states;
    utils::HashMap<Key, utils::HashMap<Key, Entry>> partitions;

    StateID cached_next_state_id = StateID::no_state;
    StateID cached_parent_id = StateID::no_state;
    OperatorID cached_creating_id = OperatorID::no_operator;
    

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit PartitionOpenList(const plugins::Options &opts);
    virtual ~PartitionOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};


template<class Entry>
void PartitionOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
}

template<class Entry>
void PartitionOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    
    cached_next_state_id = state.get_id();
    cached_parent_id = parent_state.get_id();
    cached_creating_id = op_id;
}

template<class Entry>
void PartitionOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_g = eval_context.get_g_value();
    Key next_id = cached_next_state_id.get_value();
    
    active_states.emplace(
        next_id, 
        OpenState(
            cached_next_state_id,
            cached_parent_id,
            cached_creating_id,
            -1,
            new_h,
            new_g
        ));
    Key partition_key = partition_system->insert_state(next_id, active_states);
    active_states.at(next_id).partition = partition_key;
    partitions[partition_key].emplace(next_id, entry);
    
}

template<class Entry>
Entry PartitionOpenList<Entry>::remove_min() {

    // PartitionSystem get partition until partition not empty--allows for empty partitions
    Key selected_partition_key;
    utils::HashMap<Key, Entry> selected_partition;
    do {
        selected_partition_key = partition_system->select_next_partition();
        selected_partition = partitions.at(selected_partition_key);
    } while(selected_partition.empty());


    //select node from partition system
    Key selected_state_id = partition_system->select_next_state_from_partition(selected_partition_key, active_states);
    assert(active_states.find(selected_state_id) != active_states.end());
    OpenState selected_state = active_states.at(selected_state_id);
    assert(selected_partition_key == selected_state.partition);
    assert(selected_partition.count(selected_state_id) != 0); //make sure selected partition actually contains node

    // maybe loop if entries doesn't contain it? to allow for entries and active_states to temporarily be out of sync
    Entry result = selected_partition.at(selected_state_id);
    selected_partition.erase(selected_state_id);

    // safe to remove?
    if (partition_system->safe_to_remove_node(selected_state_id)) {
        active_states.erase(selected_state_id);
    }

    if (selected_partition.empty() && partition_system->safe_to_remove_partition(selected_partition_key)) {
        partitions.erase(selected_partition_key);
    }

    return result;

}

template<class Entry>
PartitionOpenList<Entry>::PartitionOpenList(const plugins::Options &opts)
    : partition_system(opts.get<shared_ptr<PartitionSystem>>("part")),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool PartitionOpenList<Entry>::empty() const {
    return active_states.empty();
}

template<class Entry>
void PartitionOpenList<Entry>::clear() {
    active_states.clear();
    partitions.clear();
}

template<class Entry>
bool PartitionOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool PartitionOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void PartitionOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

PartitionOpenListFactory::PartitionOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionOpenList<EdgeOpenListEntry>>(options);
}

class PartitionOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionOpenListFactory> {
public:
    PartitionOpenListFeature() : TypedFeature("partition") {
        document_title("Partition Open List");
        document_synopsis("A configurable open list that selects nodes by first choosing a node parition and then choosing a node from within it. The policies for insertion (choosing a partition to insert into or creating a new one) and selection (partition first, then a node within it) are  specified policies");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<shared_ptr<PartitionSystem>>("part", "partition");
    }
};

static plugins::FeaturePlugin<PartitionOpenListFeature> _plugin;
}