#include "bts_inter_greedy_intra_epsilon.h"

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

namespace inter_greedy_intra_epsilon_open_list {
template<class Entry>
class BTSInterGreedyIntraEpOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    using Type = int;
    using TypeLoc = int;
    struct StateType {
        Type type_key;
        int h;
        Entry entry;
        StateType(Type type_key, int h, const Entry &entry) 
            : type_key(type_key), h(h), entry(entry) {};
    };
    unordered_map<int, StateType> state_types;
    unordered_map<Type, pair<TypeLoc, vector<int>>> type_buckets;
    vector<Type> type_heap;
    std::function<bool(int, int)> node_comparer;

    Type cached_parent_key;
    int cached_parent_id;
    int cached_parent_h;
    int num_types = 0;

    double inter_e;
    double intra_e;



protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    // bool node_at_index_1_bigger(const int& index1, const int& index2);  
    void adjust_type_node_up(vector<int> &heap, size_t pos);
    void adjust_type_down(vector<Type> &heap, int loc); 
    void adjust_type_up(vector<Type> &heap, int loc);   
    void move_to_top(vector<Type> &heap, int loc); 
    inline int state_h(int state_id);

public:
    explicit BTSInterGreedyIntraEpOpenList(const plugins::Options &opts);
    virtual ~BTSInterGreedyIntraEpOpenList() override = default;

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
void BTSInterGreedyIntraEpOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
    node_comparer = [&] (const int& elem1, const int& elem2) -> bool
    {
        return state_h(elem1) > state_h(elem2);
    };
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    
    StateType tmp = state_types.at(parent_state.get_id().get_value());

    cached_parent_id = parent_state.get_id().get_value();
    cached_parent_h = tmp.h;
    cached_parent_key = tmp.type_key;
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    Type type_to_insert_into;

    if (new_h < cached_parent_h) {
        type_to_insert_into = num_types;
        type_heap.push_back(type_to_insert_into);
        type_buckets.emplace(num_types, make_pair(type_buckets.size(), vector<int>{}));
        num_types+=1;
    } else {
        type_to_insert_into = cached_parent_key;
    }

    StateType new_state_type(type_to_insert_into, new_h, entry);
    state_types.emplace(new_id, new_state_type);

    type_buckets.at(type_to_insert_into).second.push_back(new_id);
    push_heap(
        type_buckets.at(type_to_insert_into).second.begin(),
        type_buckets.at(type_to_insert_into).second.end(),
        node_comparer
    );

    adjust_type_up(type_heap, type_buckets.at(type_to_insert_into).first);

}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::adjust_type_node_up(vector<int> &heap, size_t pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (state_types.at(heap[pos]).h > state_types.at(heap[parent_pos]).h) {
            break;
        }
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
inline int BTSInterGreedyIntraEpOpenList<Entry>::state_h(int state_id) {
    return state_types.at(state_id).h;
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::move_to_top(vector<Type> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        type_buckets.at(heap[pos]).first = parent_pos;
        type_buckets.at(heap[parent_pos]).first = pos;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::adjust_type_up(vector<Type> &heap, int pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (state_h(type_buckets.at(heap[parent_pos]).second[0]) < 
            state_h(type_buckets.at(heap[pos]).second[0])) {
            break;
        }
        type_buckets.at(heap[pos]).first = parent_pos;
        type_buckets.at(heap[parent_pos]).first = pos;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::adjust_type_down(vector<Type> &heap, int loc)
{
    int left_child_loc = loc * 2 + 1;
    int right_child_loc = loc * 2 + 2;
    int minimum = loc;

    if(left_child_loc < heap.size() && 
        ( state_h(type_buckets.at(heap[left_child_loc]).second[0]) < 
          state_h(type_buckets.at(heap[minimum]).second[0]) ))
            minimum = left_child_loc;

    if(right_child_loc < heap.size() && 
        ( state_h(type_buckets.at(heap[right_child_loc]).second[0]) < 
          state_h(type_buckets.at(heap[minimum]).second[0]) ))
            minimum = right_child_loc;

    if(minimum != loc) {
        type_buckets.at(heap[loc]).first = minimum;
        type_buckets.at(heap[minimum]).first = loc;
        swap(heap[loc], heap[minimum]);
        adjust_type_down(heap, minimum);
    }

}

template<class Entry>
Entry BTSInterGreedyIntraEpOpenList<Entry>::remove_min() {

    state_types.erase(cached_parent_id);
    if ( (type_buckets.find(cached_parent_key) != type_buckets.end()) 
        && type_buckets.at(cached_parent_key).second.empty()) {
        
        move_to_top(type_heap, type_buckets.at(cached_parent_key).first);
        swap(type_heap.front(), type_heap.back());
        type_heap.pop_back();
        type_buckets.at(type_heap[0]).first = 0;
        adjust_type_down(type_heap, 0);

        type_buckets.erase(cached_parent_key);
    }

    int pos = 0;
    if (rng->random() < inter_e) {
        pos = rng->random(type_heap.size());
    }

    Type type_to_remove_from = type_heap[pos];
    vector<int>& selected_bucket = type_buckets.at(type_to_remove_from).second;
    if (rng->random() < intra_e) {
        int node_pos = rng->random(selected_bucket.size());
        state_types.at(selected_bucket[node_pos]).h = numeric_limits<int>::min();
        adjust_type_node_up(selected_bucket, node_pos);
    }
    pop_heap(selected_bucket.begin(), selected_bucket.end(), node_comparer);
    int heap_node = selected_bucket.back();
    selected_bucket.pop_back();

    adjust_type_down(type_heap, pos);
    return state_types.at(heap_node).entry;

}

template<class Entry>
BTSInterGreedyIntraEpOpenList<Entry>::BTSInterGreedyIntraEpOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      inter_e(opts.get<double>("inter_e")),
      intra_e(opts.get<double>("intra_e")) {
}

template<class Entry>
bool BTSInterGreedyIntraEpOpenList<Entry>::empty() const {
    return type_heap.empty();
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::clear() {
    type_heap.clear();
    type_buckets.clear();
    state_types.clear();

    num_types = 0;
    cached_parent_h = INT32_MAX;
    cached_parent_key = -1;
}

template<class Entry>
bool BTSInterGreedyIntraEpOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterGreedyIntraEpOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterGreedyIntraEpOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterGreedyIntraEpOpenListFactory::BTSInterGreedyIntraEpOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterGreedyIntraEpOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterGreedyIntraEpOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterGreedyIntraEpOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterGreedyIntraEpOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterGreedyIntraEpOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterGreedyIntraEpOpenListFactory> {
public:
    BTSInterGreedyIntraEpOpenListFeature() : TypedFeature("bts_inter_greedy_intra_ep") {
        document_title("Type system to approximate bench transition system (BTS) and perform both inter- and intra-bench exploration");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "according to invasion percolation. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "inter_e",
            "probability for choosing the next type randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        add_option<double>(
            "intra_e",
            "probability for choosing the next node randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<BTSInterGreedyIntraEpOpenListFeature> _plugin;
}