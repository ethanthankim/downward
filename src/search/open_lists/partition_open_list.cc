#include "partition_open_list.h"

#include "partitions/partition_system.h"
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


namespace partition_open_list {
template<class Entry>
class PartitionOpenList : public OpenList<Entry> {
    shared_ptr<PartitionSystem> partition_system;
    shared_ptr<PartitionPolicy> partition_selector;
    shared_ptr<NodePolicy> node_selector;
    shared_ptr<Evaluator> evaluator;

    utils::HashMap<Key, PartitionedState> active_states;
    utils::HashMap<Key, Partition> partition_buckets;
    utils::HashMap<Key, Entry> node_entries;
    // utils::HashMap<Key, Entry> entries;

    StateID cached_next_state_id = StateID::no_state;
    StateID cached_parent_id = StateID::no_state;
    

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
    cached_parent_id = StateID::no_state;

    active_states.emplace(cached_parent_id.get_value(), 
        PartitionedState(
            cached_parent_id,
            -1,
            numeric_limits<int>::max(),
            0
        )
    );

    partition_system->notify_initial_state(initial_state);
}

template<class Entry>
void PartitionOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    
    cached_next_state_id = state.get_id();
    cached_parent_id = parent_state.get_id();

    partition_system->notify_state_transition(parent_state, op_id, state);

}

template<class Entry>
void PartitionOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_g = eval_context.get_g_value();
    Key next_id = cached_next_state_id.get_value();
    node_entries.emplace(next_id, entry);
    // empplae entry for new active state
    active_states.emplace(
        next_id, 
        PartitionedState(
            cached_next_state_id,
            -1,
            new_h,
            new_g
        ));
    Key partition_key = partition_system->choose_state_partition(active_states);
    active_states.at(next_id).partition = partition_key;

    // notify node selector insert
    node_selector->insert(next_id, active_states, partition_buckets[partition_key]);

    // then notify partition selector of a change in the partition inserted into
    partition_selector->notify_insert(next_id, active_states, partition_buckets);
}

template<class Entry>
Entry PartitionOpenList<Entry>::remove_min() {

    // PartitionSystem get partition until partition not empty--allows for empty partitions
    Key selected_partition_key;
    do {
        selected_partition_key = partition_selector->remove_min(active_states, partition_buckets);
    } while(partition_buckets.at(selected_partition_key).empty());
    Partition &selected_partition = partition_buckets.at(selected_partition_key);

    //select node from partition system
    Key selected_state_id = node_selector->remove_next_state_from_partition(active_states, selected_partition);
    PartitionedState selected_state = active_states.at(selected_state_id);

    // notify partition selector of removed node
    partition_selector->notify_remove(selected_state_id, active_states, partition_buckets);

    Entry result = node_entries.at(selected_state_id);
    node_entries.erase(selected_state_id);

    return result;

}

template<class Entry>
PartitionOpenList<Entry>::PartitionOpenList(const plugins::Options &opts)
    : partition_system(opts.get<shared_ptr<PartitionSystem>>("part")),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    partition_selector(opts.get<shared_ptr<PartitionPolicy>>("part_policy")),
    node_selector(opts.get<shared_ptr<NodePolicy>>("node_policy")) {
}

template<class Entry>
bool PartitionOpenList<Entry>::empty() const {
    return active_states.empty();
}

template<class Entry>
void PartitionOpenList<Entry>::clear() {
    active_states.clear();
    partition_buckets.clear();
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
        add_option<shared_ptr<PartitionPolicy>>("part_policy", "partition policy");
        add_option<shared_ptr<NodePolicy>>("node_policy", "node policy");
    }
};

static plugins::FeaturePlugin<PartitionOpenListFeature> _plugin;
}