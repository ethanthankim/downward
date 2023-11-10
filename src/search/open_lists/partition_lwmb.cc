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
    struct StateInfo {
        int id;
        int lwm;
        int part_key;
        StateInfo(int id, int lwm, int part_key) : id(id), lwm(lwm), part_key(part_key) {};
        StateInfo() {};
    };
    PerStateInformation<StateInfo> state_to_info;
    
    StateInfo curr_expanding_state_info;
    utils::HashMap<int, int> h_to_type;

    int next_id = 0;
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
    curr_expanding_state_info = StateInfo(-1, numeric_limits<int>::max(), -1); //type_counter starts at -1
}

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    StateInfo parent_info = state_to_info[parent_state];
    if (parent_info.id != curr_expanding_state_info.id) {
        curr_expanding_state_info = parent_info;
        h_to_type.clear();
    }

}

template<class Entry>
void PartitionLWMBBaseOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int new_lwm;
    int partition_key;
    if ( (new_h < curr_expanding_state_info.lwm) ) {
        if (h_to_type.count(new_h) == 0) {
            partition_key = type_counter++;
            h_to_type.emplace(new_h, partition_key);
        } else {
            partition_key = h_to_type.at(new_h);
        } 
        new_lwm = new_h;
    } else {
        partition_key = curr_expanding_state_info.part_key;
        new_lwm = curr_expanding_state_info.lwm;
    }

    if (key_to_partition_index.find(partition_key) == key_to_partition_index.end()) { // add empty and removed partition back (can happen with path dependent systems)
        key_to_partition_index[partition_key] = keys_and_partitions.size();
        keys_and_partitions.push_back(make_pair(partition_key, vector<Entry>{}));
    }

    int partition_index = key_to_partition_index.at(partition_key);
    keys_and_partitions[partition_index].second.push_back(entry);
    state_to_info[eval_context.get_state()] = StateInfo(next_id++, new_lwm, partition_key);
}

template<class Entry>
Entry PartitionLWMBBaseOpenList<Entry>::remove_min() {

    size_t bucket_index = rng->random(keys_and_partitions.size());
    pair<int, vector<Entry>> &key_and_partition = keys_and_partitions[bucket_index];
    const int &min_key = key_and_partition.first;
    vector<Entry> &bucket = key_and_partition.second;
    int pos = rng->random(bucket.size());
    Entry result = utils::swap_and_pop_from_vector(bucket, pos);

    if (bucket.empty()) {
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_partition_index[keys_and_partitions.back().first] = bucket_index;
        key_to_partition_index.erase(min_key);
        utils::swap_and_pop_from_vector(keys_and_partitions, bucket_index);
    }
    return result;

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
    PartitionLWMBBaseOpenListFeature() : TypedFeature("lwmb") {
        document_title("Partition LWM Open List");
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