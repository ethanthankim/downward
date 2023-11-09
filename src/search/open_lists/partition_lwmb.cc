#include "partition_lwmb.h"

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
#include <map>
#include <vector>

using namespace std;

namespace partition_lwmb_base_open_list {
template<class Entry>
class PartitionLWMBBaseOpenList : public OpenList<Entry> {

    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    vector<pair<int, vector<Entry>>> keys_and_partitions;
    utils::HashMap<int, int> key_to_partition_index;
    int type_counter = 0;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit PartitionLWMBBaseOpenList(const plugins::Options &opts);
    virtual ~PartitionLWMBBaseOpenList() override = default;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;

};

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::notify_initial_state(const State &initial_state) {
    
}

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{

}

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    
}

template<class Entry>
Entry PartitionLWMBBaseOpenList<Entry>::remove_min() {

   

}


template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::clear() {
    keys_and_partitions.clear();
    key_to_partition_index.clear();
}

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool PartitionLWMBBaseOpenList<Entry>::empty() const {
    return keys_and_partitions.empty();
}

template<class Entry>
bool PartitionLWMBBaseOpenList<Entry>::is_dead_end(EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool PartitionLWMBBaseOpenList<Entry>::is_reliable_dead_end(EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
PartitionLWMBBaseOpenList<Entry>::PartitionLWMBBaseOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      type_counter(0) {
}

PartitionLWMBBaseOpenListFactory::PartitionLWMBBaseOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionLWMBBaseOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionLWMBBaseOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionLWMBBaseOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionLWMBBaseOpenList<EdgeOpenListEntry>>(options);
}

class PartitionLWMBBaseOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionLWMBBaseOpenListFactory> {
public:
    PartitionLWMBBaseOpenListFeature() : TypedFeature("bias_depth_bias_h") {
        document_title("Partition Heuristic Improvement Open List");
        document_synopsis("A configurable open list that selects nodes by first"
         "choosing a node parition and then choosing a node from within it."
         "The policies for insertion (choosing a partition to insert into or"
         "creating a new one) and selection (partition first, then a node within it)"
         "are  specified policies");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<PartitionLWMBBaseOpenListFeature> _plugin;
}