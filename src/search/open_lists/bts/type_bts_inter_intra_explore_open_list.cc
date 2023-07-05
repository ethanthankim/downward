#include "type_bts_inter_intra_explore_open_list.h"

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

namespace type_bts_intra_explore_open_list {
template<class Entry>
class BTSIntraExploreOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct TypeInfo {
        int h;
        int depth;
        int type_key;
    };
    struct HeapNode {
        int h;
        int depth;
        Entry entry;
        HeapNode(int h, const Entry &entry)
            : h(h), entry(entry) {
        }

        bool operator>(const HeapNode &other) const {
            if (h == other.h) return depth < other.depth;
            return h > other.h;
        }
    };

    using Key = int;
    using Bucket = vector<HeapNode>;
    PerStateInformation<TypeInfo> state_type_infos;
    vector<pair<Key, Bucket>> keys_and_buckets;
    utils::HashMap<Key, int> key_to_bucket_index;

    TypeInfo cached_parent_info;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    TypeInfo insert_type_info(EvaluationContext &eval_context);

public:
    explicit BTSIntraExploreOpenList(const plugins::Options &opts);
    virtual ~BTSIntraExploreOpenList() override = default;

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
void BTSIntraExploreOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_info = {INT32_MAX, -1};
    state_type_infos[initial_state] = {-1, 0, 0};
}

template<class Entry>
void BTSIntraExploreOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    cached_parent_info = state_type_infos[parent_state];
}

template<class Entry>
BTSIntraExploreOpenList<Entry>::TypeInfo BTSIntraExploreOpenList<Entry>::insert_type_info(EvaluationContext &eval_context) {

    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();

    int new_type_key, new_depth;
    if (new_h < cached_parent_info.h) { 
        // if the new node is a new local minimum, it gets a new bucket
        new_type_key = new_id;
        new_depth = cached_parent_info.depth + 1;
    } else {
        // if the new node isn't a new local minimum, it gets bucketted with its parent
        new_type_key = cached_parent_info.type_key;
        new_depth = cached_parent_info.depth;
    }
    TypeInfo new_info = { new_h, new_depth, new_type_key };
    state_type_infos[eval_context.get_state()] = new_info;

    return new_info;
}

template<class HeapNode>
static void adjust_heap_up(vector<HeapNode> &heap, size_t pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (heap[pos] > heap[parent_pos]) {
            break;
        }
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSIntraExploreOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    TypeInfo info = insert_type_info(eval_context);
    // int r = rng->random(info.h + 1);
    HeapNode new_node(info.h, entry);
    auto it = key_to_bucket_index.find(info.type_key);
    if (it == key_to_bucket_index.end()) {
        key_to_bucket_index[info.type_key] = keys_and_buckets.size();
        keys_and_buckets.push_back(make_pair(info.type_key, Bucket({new_node})));
    } else {
        size_t bucket_index = it->second;
        assert(utils::in_bounds(bucket_index, keys_and_buckets));
        keys_and_buckets[bucket_index].second.push_back(new_node);
        push_heap(keys_and_buckets[bucket_index].second.begin(), keys_and_buckets[bucket_index].second.end(), greater<HeapNode>());
    }
}

template<class Entry>
Entry BTSIntraExploreOpenList<Entry>::remove_min() {
    size_t bucket_id = rng->random(keys_and_buckets.size());
    auto &key_and_bucket = keys_and_buckets[bucket_id];
    const Key &min_key = key_and_bucket.first;
    Bucket &bucket = key_and_bucket.second;

    pop_heap(bucket.begin(), bucket.end(), greater<HeapNode>());
    HeapNode heap_node = bucket.back();
    bucket.pop_back();

    if (bucket.empty()) { 
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_bucket_index[keys_and_buckets.back().first] = bucket_id;
        key_to_bucket_index.erase(min_key);
        utils::swap_and_pop_from_vector(keys_and_buckets, bucket_id);  
    }

    return heap_node.entry;
}

template<class Entry>
BTSIntraExploreOpenList<Entry>::BTSIntraExploreOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool BTSIntraExploreOpenList<Entry>::empty() const {
    return keys_and_buckets.empty();
}

template<class Entry>
void BTSIntraExploreOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
}

template<class Entry>
bool BTSIntraExploreOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSIntraExploreOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSIntraExploreOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSIntraExploreOpenListFactory::BTSIntraExploreOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSIntraExploreOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSIntraExploreOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSIntraExploreOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSIntraExploreOpenList<EdgeOpenListEntry>>(options);
}

class BTSIntraExploreOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSIntraExploreOpenListFactory> {
public:
    BTSIntraExploreOpenListFeature() : TypedFeature("bts_intra_explore") {
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

static plugins::FeaturePlugin<BTSIntraExploreOpenListFeature> _plugin;
}