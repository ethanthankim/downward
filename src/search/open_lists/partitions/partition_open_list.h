#ifndef OPEN_LISTS_PARTITION_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_OPEN_LIST_H

#include "../../evaluator.h"
#include "../../open_list.h"

#include "../../utils/collections.h"
#include "../../utils/hash.h"

#include "inter/partition_policy.h"
#include "intra/node_policy.h"

#include <memory>
#include <limits>
#include <set>

namespace partition_open_list {
struct PartitionedNode {
    int partition;
    int eval;
    PartitionedNode(int partition, int eval)
        : partition(partition), eval(eval) {
    }
};

template<class Entry>
class PartitionOpenList : public OpenList<Entry> {

protected:
    void partition_insert(EvaluationContext & eval_context, int eval, Entry entry, int partition_key, bool new_partition);

public:
    std::shared_ptr<PartitionPolicy> partition_selector;
    std::shared_ptr<NodePolicy> node_selector;
    std::shared_ptr<Evaluator> evaluator;

    utils::HashMap<int, PartitionedNode> partitioned_nodes; // Entry should really be member of PartitionedState
    utils::HashMap<int, Entry> entries; 

    StateID cached_next_state_id = StateID::no_state;
    StateID cached_parent_id = StateID::no_state;
    int last_chosen_node = StateID::no_state.get_value();
    int last_chosen_partition = -1;

    explicit PartitionOpenList(
        const std::shared_ptr<Evaluator>& evaluator,
        const std::shared_ptr<PartitionPolicy>& partition_policy,
        const std::shared_ptr<NodePolicy>& node_policy);
    virtual ~PartitionOpenList() = default;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;

    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) = 0;
    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;

};

template<class Entry>
PartitionOpenList<Entry>::PartitionOpenList(
        const std::shared_ptr<Evaluator>& evaluator,
        const std::shared_ptr<PartitionPolicy>& partition_policy,
        const std::shared_ptr<NodePolicy>& node_policy)
            : evaluator(evaluator),
            node_selector(node_policy),
            partition_selector(partition_policy) {}

template<class Entry>
void PartitionOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_next_state_id = initial_state.get_id();
    partitioned_nodes.emplace(
        cached_parent_id.get_value(),
        PartitionedNode(
            cached_parent_id.get_value(),
            std::numeric_limits<int>::max()
        )
    );
}

template<class Entry>
void PartitionOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    cached_next_state_id = state.get_id();
    cached_parent_id = parent_state.get_id();
}

template<class Entry>
void PartitionOpenList<Entry>::partition_insert(
        EvaluationContext &eval_context, 
        int eval,
        Entry entry, 
        int partition_key,
        bool new_partition
    ) 
{
    int node_key = eval_context.get_state().get_id().get_value();
    partitioned_nodes.emplace( 
        node_key,
        PartitionedNode(
            partition_key,
            eval
        )
    );

    entries.emplace(
        node_key,
        entry
    );

    partition_selector->notify_insert(partition_key, node_key, new_partition, eval_context);
    node_selector->notify_insert(partition_key, node_key, new_partition, eval_context);
}

template<class Entry>
Entry PartitionOpenList<Entry>::remove_min() {

    partitioned_nodes.erase(last_chosen_node);
    entries.erase(last_chosen_node);

    int chosen_partition = partition_selector->get_next_partition();
    int chosen_node = node_selector->get_next_node(chosen_partition);
    partition_selector->notify_removal(chosen_partition, chosen_node);

    Entry result = entries.at(chosen_node);
    last_chosen_node = chosen_node;
    last_chosen_partition = chosen_partition;

    return result;

}

template<class Entry>
void PartitionOpenList<Entry>::get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
    partition_selector->get_path_dependent_evaluators(evals);
    node_selector->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool PartitionOpenList<Entry>::empty() const {
    return partitioned_nodes.empty();
}

template<class Entry>
void PartitionOpenList<Entry>::clear() {
    partitioned_nodes.clear();
    partition_selector->clear();
    node_selector->clear();
}

template<class Entry>
bool PartitionOpenList<Entry>::is_dead_end(EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool PartitionOpenList<Entry>::is_reliable_dead_end(EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

}

#endif

