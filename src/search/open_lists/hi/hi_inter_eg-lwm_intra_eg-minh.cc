#include "hi_inter_eg-lwm_intra_eg-minh.h"

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
#include <map>
#include <fstream>

using namespace std;

namespace inter_lwm_intra_epsilon_open_list {
template<class Entry>
class BTSInterLWMIntraEpOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    using Key = int;
    using Index = int;
    struct StateType {
        Key type_key;
        int h;
        Entry entry;
        StateType(Key type_key, int h, const Entry &entry) 
            : type_key(type_key), h(h), entry(entry) {};
    };
    unordered_map<int, StateType> state_types;
    
    struct TypeBucket {
        Index type_heap_index;
        int type_h;
        vector<Key> bucket;
        TypeBucket(Index type_heap_index, int h, const vector<Key> &bucket) 
            : type_heap_index(type_heap_index), type_h(h), bucket(bucket) {};
    };
    unordered_map<Key, TypeBucket> type_buckets;
    vector<Key> type_heap;

    Key cached_parent_key;
    int cached_parent_id;
    int cached_parent_h;
    int num_types = 0;

    double inter_e;
    double intra_e;



protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    using Synchronizer = void (BTSInterLWMIntraEpOpenList<Entry>::*)(vector<Key>&, int, int);
    using Comparer = bool (BTSInterLWMIntraEpOpenList<Entry>::*)(vector<Key>&, int, int);
    void adjust_heap_down(vector<Key> &heap, int loc, Comparer, Synchronizer = NULL); 
    void adjust_heap_up(vector<Key> &heap, int loc, Comparer, Synchronizer = NULL);   
    void adjust_to_top(vector<Key> &heap, int loc, Synchronizer = NULL); 
    Key random_access_heap_pop(vector<Key> &heap, int loc, Comparer, Synchronizer = NULL);

    bool compare_parent_node_smaller(vector<Key>& heap, int parent, int child);
    bool compare_parent_node_bigger(vector<Key>& heap, int parent, int child);
    bool compare_parent_type_smaller(vector<Key>& heap, int parent, int child);
    bool compare_parent_type_bigger(vector<Key>& heap, int parent, int child);
    void sync_type_heap_and_type_location(vector<Key> &heap, int pos1, int pos2);

    inline int state_h(int state_id);

public:
    explicit BTSInterLWMIntraEpOpenList(const plugins::Options &opts);
    virtual ~BTSInterLWMIntraEpOpenList() override = default;

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
void BTSInterLWMIntraEpOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    
    StateType tmp = state_types.at(parent_state.get_id().get_value());

    cached_parent_id = parent_state.get_id().get_value();
    cached_parent_h = tmp.h;
    cached_parent_key = tmp.type_key;
}

template<class Entry>
inline int BTSInterLWMIntraEpOpenList<Entry>::state_h(int state_id) {
    return state_types.at(state_id).h;
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::compare_parent_node_smaller(vector<Key>& heap, int parent, int child) {
    return state_h(heap[parent]) < state_h(heap[child]);
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::compare_parent_node_bigger(vector<Key>& heap, int parent, int child) {
    return state_h(heap[parent]) > state_h(heap[child]);
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::compare_parent_type_smaller(vector<Key>& heap, int parent, int child) {
    if (type_buckets.at(heap[parent]).bucket.empty()) return false;
    if (type_buckets.at(heap[child]).bucket.empty()) return true;
    return type_buckets.at(heap[parent]).type_h < type_buckets.at(heap[child]).type_h;
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::compare_parent_type_bigger(vector<Key>& heap, int parent, int child) {
    if (type_buckets.at(heap[parent]).bucket.empty()) return true;
    if (type_buckets.at(heap[child]).bucket.empty()) return false;
    return type_buckets.at(heap[parent]).type_h > type_buckets.at(heap[child]).type_h;
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::sync_type_heap_and_type_location(vector<Key> &heap, int pos1, int pos2) {
    type_buckets.at(heap[pos2]).type_heap_index = pos1;
    type_buckets.at(heap[pos1]).type_heap_index = pos2;
}

template<class Entry>
BTSInterLWMIntraEpOpenList<Entry>::Key BTSInterLWMIntraEpOpenList<Entry>::random_access_heap_pop(
        vector<Key> &heap, 
        int loc, 
        Comparer comp, 
        Synchronizer sync) 
{
    adjust_to_top(
        heap, 
        loc,
        sync
    );
    swap(heap.front(), heap.back());
    if (sync) (this->*sync)(heap, 0, 0);

    Key to_return = heap.back();
    heap.pop_back();

    adjust_heap_down(heap, 0, comp, sync);
    return to_return;
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::adjust_to_top(vector<Key> &heap, int pos, Synchronizer sync) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (sync) (this->*sync)(heap, pos, parent_pos);
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::adjust_heap_up(
        vector<Key> &heap, 
        int pos, 
        Comparer comp,
        Synchronizer sync) 
{
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ((this->*comp)(heap, parent_pos, pos))
            break;
        if (sync) (this->*sync)(heap, parent_pos, pos);
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::adjust_heap_down(
        vector<Key> &heap, 
        int loc, 
        Comparer comp,
        Synchronizer sync)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && ( (this->*comp)(heap, minimum, left_child_loc) ))
            minimum = left_child_loc;

    if(right_child_loc < heap.size() && ( (this->*comp)(heap, minimum, right_child_loc) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        if (sync) (this->*sync)(heap, loc, minimum);
        swap(heap[loc], heap[minimum]);
        adjust_heap_down(heap, minimum, comp, sync);
    }

}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    Key type_to_insert_into;

    if (new_h < cached_parent_h) {
        type_to_insert_into = num_types;
        type_heap.push_back(type_to_insert_into);
        type_buckets.emplace(type_to_insert_into, TypeBucket(type_heap.size()-1, new_h, vector<int>{}));
        num_types+=1;
    } else {
        type_to_insert_into = cached_parent_key;
    }

    StateType new_state_type(type_to_insert_into, new_h, entry);
    state_types.emplace(new_id, new_state_type);

    type_buckets.at(type_to_insert_into).bucket.push_back(new_id);
    adjust_heap_up(
        type_buckets.at(type_to_insert_into).bucket,
        type_buckets.at(type_to_insert_into).bucket.size()-1,
        &BTSInterLWMIntraEpOpenList<Entry>::compare_parent_node_smaller
    );

    adjust_heap_up(
        type_heap, 
        type_buckets.at(type_to_insert_into).type_heap_index, 
        &BTSInterLWMIntraEpOpenList<Entry>::compare_parent_type_smaller,
        &BTSInterLWMIntraEpOpenList<Entry>::sync_type_heap_and_type_location
    );

}

template<class Entry>
Entry BTSInterLWMIntraEpOpenList<Entry>::remove_min() {

    state_types.erase(cached_parent_id);
    if ( (type_buckets.find(cached_parent_key) != type_buckets.end()) 
        && type_buckets.at(cached_parent_key).bucket.empty()) {
        
        Key erase_key = random_access_heap_pop(
            type_heap, 
            type_buckets.at(cached_parent_key).type_heap_index,
            &BTSInterLWMIntraEpOpenList<Entry>::compare_parent_type_bigger,
            &BTSInterLWMIntraEpOpenList<Entry>::sync_type_heap_and_type_location
        );
        type_buckets.erase(cached_parent_key);
    }

    int pos = 0;
    if (rng->random() < inter_e) {
        pos = rng->random(type_heap.size());
    }

    Key type_to_remove_from = type_heap[pos];
    vector<int>& selected_bucket = type_buckets.at(type_to_remove_from).bucket;
    Key selected_node = random_access_heap_pop(
        selected_bucket, 
        rng->random() < intra_e ? rng->random(selected_bucket.size()) : 0, // epsilon greedy selection
        &BTSInterLWMIntraEpOpenList<Entry>::compare_parent_node_bigger
    );

    adjust_heap_down(
        type_heap, 
        pos, 
        &BTSInterLWMIntraEpOpenList<Entry>::compare_parent_type_bigger,
        &BTSInterLWMIntraEpOpenList<Entry>::sync_type_heap_and_type_location
    );
    return state_types.at(selected_node).entry;

}

template<class Entry>
BTSInterLWMIntraEpOpenList<Entry>::BTSInterLWMIntraEpOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      inter_e(opts.get<double>("inter_e")),
      intra_e(opts.get<double>("intra_e")) {
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::empty() const {
    return type_heap.empty();
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::clear() {
    type_heap.clear();
    type_buckets.clear();
    state_types.clear();

    num_types = 0;
    cached_parent_h = INT32_MAX;
    cached_parent_key = -1;
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterLWMIntraEpOpenListFactory::BTSInterLWMIntraEpOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterLWMIntraEpOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterLWMIntraEpOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterLWMIntraEpOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterLWMIntraEpOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterLWMIntraEpOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterLWMIntraEpOpenListFactory> {
public:
    BTSInterLWMIntraEpOpenListFeature() : TypedFeature("hi_lwm") {
        document_title("Type system to approximate bench transition system (BTS) and perform both inter- and intra-bench exploration");
        document_synopsis("TODO");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "inter_e",
            "probability of choosing the next type randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        add_option<double>(
            "intra_e",
            "probability of choosing the next node randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<BTSInterLWMIntraEpOpenListFeature> _plugin;
}