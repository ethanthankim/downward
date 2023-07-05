#include "type_bts_inter_explore_open_list.h"

#include "../../evaluator.h"
#include "../../open_list.h"

#include "../../plugins/plugin.h"
#include "../../utils/collections.h"
#include "../../utils/hash.h"
#include "../../utils/markup.h"
#include "../../utils/memory.h"
#include "../../utils/rng.h"
#include "../../utils/rng_options.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <deque>
#include <fstream>

using namespace std;

namespace type_bts_inter_explore_open_list {
template<class Entry>
class BTSInterExploreOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    using Bucket = vector<Entry>;
    vector<int> type_heap;
    struct TypeInfo {
        int type_key;
        int h;
        int depth;
        TypeInfo(int type_key, int h, int depth)
            : type_key(type_key), h(h), depth(depth) {
        }
    };
    PerStateInformation<pair<int, int>> state_type_index_and_h;
    vector<pair<TypeInfo, Bucket>> type_buckets;
    vector<int> new_type_root_keys;

    int cached_parent_type_key;
    int cached_parent_h;
    int cached_parent_depth;

    bool doing_initial;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool type_comparator(const int& index1, const int& index2);

public:
    explicit BTSInterExploreOpenList(const plugins::Options &opts);
    virtual ~BTSInterExploreOpenList() override = default;

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
void BTSInterExploreOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
    cached_parent_type_key = -1;
    cached_parent_depth = -1;
    doing_initial = true;
}

template<class Entry>
void BTSInterExploreOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    pair<int, int> parent_info = state_type_index_and_h[parent_state];
    cached_parent_h = parent_info.first;
    cached_parent_type_key = parent_info.second;
    cached_parent_depth = type_buckets[cached_parent_type_key].first.depth;
}

template<class Entry>
bool BTSInterExploreOpenList<Entry>::type_comparator(const int& index1, const int& index2) {
    TypeInfo first_type = type_buckets[index1].first;
    TypeInfo second_type = type_buckets[index2].first;

    if (first_type.h == second_type.h) return first_type.depth < second_type.depth;
    return first_type.h > second_type.h;
}

template<class Entry>
void BTSInterExploreOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    if (new_h < cached_parent_h) { 
        // if the new node is a new local minimum, it gets a new bucket 
        int type_index = type_buckets.size();
        if (doing_initial) {
            type_heap.push_back(type_index);
            doing_initial = false;
        } else {
            new_type_root_keys.push_back(type_index);
        }
        TypeInfo info(type_index, new_h, cached_parent_depth + 1);
        Bucket new_type_bucket({entry});
        type_buckets.push_back(make_pair(info, new_type_bucket));
        state_type_index_and_h[eval_context.get_state()] = make_pair(new_h, type_index);
    } else {
        // if the new node isn't a new local minimum, it gets bucketted with its parent
        pair<TypeInfo, Bucket> &type_bucket = type_buckets[cached_parent_type_key];
        type_bucket.second.push_back(entry);
        state_type_index_and_h[eval_context.get_state()] = make_pair(new_h, cached_parent_type_key);
    }
}

template<class Entry>
Entry BTSInterExploreOpenList<Entry>::remove_min() {

    auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
    {
        return type_comparator(elem1, elem2);
    };
    if (type_buckets[type_heap.front()].second.empty()) { 
        // The previous type remains empty following insertions, that type will therefore
        // be empty for the rest of the search and can now safely be removed from the heap.
        pop_heap(type_heap.begin(), type_heap.end(), heap_compare);
        type_heap.pop_back(); 
    }

    // insert all the new types from the previous inserts into the heap
    for (int root_i : new_type_root_keys) {
        type_heap.push_back(root_i);
        push_heap(type_heap.begin(), type_heap.end(), heap_compare);
    }
    new_type_root_keys.clear();


    pair<TypeInfo, Bucket> &min_type = type_buckets[type_heap.front()];
    Bucket &bucket = min_type.second;
    int pos = rng->random(bucket.size());
    Entry result = utils::swap_and_pop_from_vector(bucket, pos);

    return result;
}

template<class Entry>
BTSInterExploreOpenList<Entry>::BTSInterExploreOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool BTSInterExploreOpenList<Entry>::empty() const {
    return type_buckets.empty();
}

template<class Entry>
void BTSInterExploreOpenList<Entry>::clear() {
    type_buckets.clear();
    type_heap.clear();
}

template<class Entry>
bool BTSInterExploreOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterExploreOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterExploreOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterExploreOpenListFactory::BTSInterExploreOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterExploreOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterExploreOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterExploreOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterExploreOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterExploreOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterExploreOpenListFactory> {
public:
    BTSInterExploreOpenListFeature() : TypedFeature("bts_inter_explore") {
        document_title("Type system to approximate bench transition system (BTS) and perform both inter- and intra-bench exploration");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "according to invasion percolation. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<BTSInterExploreOpenListFeature> _plugin;
}