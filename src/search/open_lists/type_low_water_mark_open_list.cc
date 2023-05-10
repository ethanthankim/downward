#include "type_low_water_mark_open_list.h"

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

namespace lwm_based_open_list {
template<class Entry>
class LWMBasedOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;
    struct TypeInfo {
        int h;
        int type_key;
    };
    using Key = int;
    using Bucket = vector<Entry>;

    // TODO: use PerStateInformation for state_type_infos
    utils::HashMap<Key, TypeInfo> state_type_infos;
    vector<pair<Key, Bucket>> keys_and_buckets;
    utils::HashMap<Key, int> key_to_bucket_index;

    int parent_cache;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    TypeInfo insert_type_info(int new_h, int new_id);

public:
    explicit LWMBasedOpenList(const plugins::Options &opts);
    virtual ~LWMBasedOpenList() override = default;

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
void LWMBasedOpenList<Entry>::notify_initial_state(const State &initial_state) {
    
    // using no_state as a parent to the initial feels both stupid and not...
    parent_cache = StateID::no_state.get_value();
    state_type_infos[parent_cache] = {INT32_MAX, -1};
    key_to_bucket_index[parent_cache] = 0;
}

template<class Entry>
void LWMBasedOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    parent_cache = parent_state.get_id().get_value();
}

template<class Entry>
LWMBasedOpenList<Entry>::TypeInfo LWMBasedOpenList<Entry>::insert_type_info(int new_h, int new_id) {

    TypeInfo parent_type_info = state_type_infos[parent_cache];

    int new_type_h, new_type_key;
    if (new_h < parent_type_info.h) { 
        // if the new node is a new local minimum, it gets a new bucket
        new_type_h = new_h;
        new_type_key = new_id;
    } else {
        // if the new node isn't a new local minimum, it gets bucketted with its parent
        new_type_h = parent_type_info.h;
        new_type_key = parent_type_info.type_key;
    }
    TypeInfo new_info = { new_type_h, new_type_key };
    state_type_infos[new_id] = new_info;

    return new_info;
}

template<class Entry>
void LWMBasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    TypeInfo info = insert_type_info(new_h, new_id);
    
    auto it = key_to_bucket_index.find(info.type_key);
    if (it == key_to_bucket_index.end()) {
        key_to_bucket_index[info.type_key] = keys_and_buckets.size();
        keys_and_buckets.push_back(make_pair(info.type_key, Bucket({entry})));
    } else {
        size_t bucket_index = it->second;
        assert(utils::in_bounds(bucket_index, keys_and_buckets));
        keys_and_buckets[bucket_index].second.push_back(entry);
    }
}

template<class Entry>
Entry LWMBasedOpenList<Entry>::remove_min() {
    size_t bucket_id = rng->random(keys_and_buckets.size());
    auto &key_and_bucket = keys_and_buckets[bucket_id];
    const Key &min_key = key_and_bucket.first;
    Bucket &bucket = key_and_bucket.second;
    int pos = rng->random(bucket.size());
    Entry result = utils::swap_and_pop_from_vector(bucket, pos);

    if (bucket.empty()) { 
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_bucket_index[keys_and_buckets.back().first] = bucket_id;
        key_to_bucket_index.erase(min_key);
        utils::swap_and_pop_from_vector(keys_and_buckets, bucket_id);  
    }
    return result;
}

template<class Entry>
LWMBasedOpenList<Entry>::LWMBasedOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool LWMBasedOpenList<Entry>::empty() const {
    return keys_and_buckets.empty();
}

template<class Entry>
void LWMBasedOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
}

template<class Entry>
bool LWMBasedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool LWMBasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void LWMBasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

LWMBasedOpenListFactory::LWMBasedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LWMBasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LWMBasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LWMBasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LWMBasedOpenList<EdgeOpenListEntry>>(options);
}

class LWMBasedOpenListFeature : public plugins::TypedFeature<OpenListFactory, LWMBasedOpenListFactory> {
public:
    LWMBasedOpenListFeature() : TypedFeature("lwm_based_type") {
        document_title("Low water mark based type system open list");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "uniformly randomly. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<LWMBasedOpenListFeature> _plugin;
}