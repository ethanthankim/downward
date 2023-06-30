#include "type_bts_intra_depth_open_list.h"

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
#include <deque>
#include <fstream>

using namespace std;

namespace type_intra_depth_open_list {
template<class Entry>
class BTSIntraDepthOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct HeapNode {
        int h;
        int depth;
        Entry entry;
        HeapNode(int id, int h, const Entry &entry)
            : id(id), h(h), entry(entry) {
        }

        bool operator>(const TypeBucket &other) const {
            if (h == other.h) return depth > other.depth;
            return h > other.h;
        }
    };
    struct TypeBucket {
        int type_key;
        int h;
        int depth;
        vector<HeapNode> nodes;
        TypeBucket(int type_key, int h, int depth, const vector<HeapNode> &nodes)
            : type_key(type_key), h(h), depth(depth), nodes(nodes) {
        }
    };
    PerStateInformation<pair<int, int>> state_type_index_and_h;
    vector<TypeBucket> type_buckets;
    // i think need key to type index.
    // remove_min still doesn't work. 

    int cached_parent_depth;
    int cached_parent_type_depth;
    int cached_parent_h;
    int cached_parent_type_key;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit BTSIntraDepthOpenList(const plugins::Options &opts);
    virtual ~BTSIntraDepthOpenList() override = default;

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
void BTSIntraDepthOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_info = {INT32_MAX, -1};
    state_type_infos[initial_state] = {-1, 0, 0};
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    cached_parent_info = state_type_infos[parent_state];
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
void BTSIntraDepthOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    int type_index;

    if (new_h < cached_parent_h) { 
        // if the new node is a new local minimum, it gets a new bucket
        type_index = type_buckets.size();
        HeapNode new_node(new_h, cached_parent_depth + 1, entry);
        TypeBucket new_bucket(type_index, new_h, cached_parent_type_depth + 1, {new_node});
        type_buckets.push_back(new_bucket);
    } else {
        // if the new node isn't a new local minimum, it gets bucketted with its parent
        type_index = cached_parent_type_key;
        HeapNode new_node(new_h, cached_parent_depth + 1, entry);
        type_buckets[cached_parent_type_key].nodes.push_back(new_node);
        push_heap(type_buckets[cached_parent_type_key].nodes.begin(), type_buckets[cached_parent_type_key].nodes.end(), greater<HeapNode>());
    }
    state_type_index_and_h[eval_context.get_state()] = make_pair(new_h, type_index);
}

template<class Entry>
Entry BTSIntraDepthOpenList<Entry>::remove_min() {
    size_t bucket_id = rng->random(type_buckets.size());
    TypeBucket &type_bucket = type_buckets[bucket_id];
    vector<HeapNode> &bucket = type_bucket.nodes; 

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
BTSIntraDepthOpenList<Entry>::BTSIntraDepthOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool BTSIntraDepthOpenList<Entry>::empty() const {
    return keys_and_buckets.empty();
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
}

template<class Entry>
bool BTSIntraDepthOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSIntraDepthOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSIntraDepthOpenListFactory::BTSIntraDepthOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSIntraDepthOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSIntraDepthOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSIntraDepthOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSIntraDepthOpenList<EdgeOpenListEntry>>(options);
}

class BTSIntraDepthOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSIntraDepthOpenListFactory> {
public:
    BTSIntraDepthOpenListFeature() : TypedFeature("bts_intra_depth_type") {
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

static plugins::FeaturePlugin<BTSIntraDepthOpenListFeature> _plugin;
}