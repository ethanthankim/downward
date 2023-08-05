#include "hi_inter_biased_intra_any.h"

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

namespace inter_biased_intra_any {
template<class Entry>
class BTSInterBiasedIntraAnyOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> inter_evaluator;
    shared_ptr<Evaluator> intra_evaluator;

    int size;
    double tau;
    double epsilon;
    double current_sum;

    using Key = int;
    using Index = int;
    struct StateType {
        Key type_key;
        int inter_h;
        int intra_h;
        Entry entry;
        StateType(Key type_key, int inter_h, int intra_h, const Entry &entry) 
            : type_key(type_key), inter_h(inter_h), intra_h(intra_h), entry(entry) {};
    };
    map<int, deque<Key>> states;
    unordered_map<Key, StateType> state_types;
    unordered_map<Key, vector<Key>> type_buckets;

    int cached_parent_h;
    Key cached_parent_type_bucket;
    int cached_parent_id;
    int num_types = 0;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;
    
    using Comparer = bool (BTSInterBiasedIntraAnyOpenList<Entry>::*)(vector<Key>&, int, int);
    bool compare_parent_node_smaller(vector<Key>& heap, int parent, int child);
    bool compare_parent_node_bigger(vector<Key>& heap, int parent, int child);
    void adjust_heap_up(vector<Key> &heap, int pos, Comparer comp);

public:
    explicit BTSInterBiasedIntraAnyOpenList(const plugins::Options &opts);
    virtual ~BTSInterBiasedIntraAnyOpenList() override = default;

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
void BTSInterBiasedIntraAnyOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
    cached_parent_type_bucket = -1;

}

template<class Entry>
void BTSInterBiasedIntraAnyOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    StateType parent = state_types.at(parent_state.get_id().get_value());
    cached_parent_type_bucket = parent.type_key;
    cached_parent_h = parent.inter_h;
    cached_parent_id = parent_state.get_id().get_value();
}

template<class Entry>
bool BTSInterBiasedIntraAnyOpenList<Entry>::compare_parent_node_smaller(vector<Key>& heap, int parent, int child) {
    return state_types.at(heap[parent]).intra_h < state_types.at(heap[child]).intra_h;
}

template<class Entry>
bool BTSInterBiasedIntraAnyOpenList<Entry>::compare_parent_node_bigger(vector<Key>& heap, int parent, int child) {
    return state_types.at(heap[parent]).intra_h > state_types.at(heap[child]).intra_h;
}

template<class Entry>
void BTSInterBiasedIntraAnyOpenList<Entry>::adjust_heap_up(
        vector<Key> &heap, 
        int pos, 
        Comparer comp) 
{
    if (pos<0) return;
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if ((this->*comp)(heap, parent_pos, pos))
            break;
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void BTSInterBiasedIntraAnyOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_inter_h = eval_context.get_evaluator_value_or_infinity(inter_evaluator.get());
    int new_intra_h = eval_context.get_evaluator_value_or_infinity(intra_evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    Key type_to_insert_into;

    if (new_inter_h < cached_parent_h) {
        type_to_insert_into = num_types;
        type_buckets.emplace(type_to_insert_into, vector<int>{});
        num_types+=1;
    } else {
        type_to_insert_into = cached_parent_type_bucket;
    }

    StateType new_state_type(type_to_insert_into, new_inter_h, new_intra_h, entry);
    state_types.emplace(new_id, new_state_type);
    states[new_inter_h].push_back(new_id);
    current_sum += std::exp(-1.0 * static_cast<double>(new_inter_h) / tau);

    type_buckets.at(type_to_insert_into).push_back(new_id);
    adjust_heap_up(
        type_buckets.at(type_to_insert_into),
        type_buckets.at(type_to_insert_into).size()-1,
        &BTSInterBiasedIntraAnyOpenList<Entry>::compare_parent_node_smaller
    );
    size++;
}

template<class Entry>
Entry BTSInterBiasedIntraAnyOpenList<Entry>::remove_min() {

    // state_types.erase(cached_parent_id);
    if ( (type_buckets.find(cached_parent_type_bucket) != type_buckets.end()) 
        && type_buckets.at(cached_parent_type_bucket).empty()) {
        type_buckets.erase(cached_parent_type_bucket);
    }

    assert(size > 0);
    int key = states.begin()->first;

    if (states.size() > 1) {
        double r = rng->random();
        if (r <= epsilon) {
            double total_sum = current_sum;
            double p_sum = 0.0;
            for (auto it : states) {
                double p = 1.0 / total_sum;
                p *= std::exp(-1.0 * static_cast<double>(it.first) / tau);
                p *= static_cast<double>(it.second.size());
                p_sum += p;
                if (r <= p_sum * epsilon) {
                    key = it.first;
                    break;
                }
            }
        }
    }


    auto &state_queue = states[key];
    assert(!state_queue.empty());
    Key state_key;
        state_key = state_queue.front();
        state_queue.pop_front();
    if (state_queue.empty()) {
        states.erase(key);
    }
    current_sum -= std::exp(-1.0 * static_cast<double>(key) / tau);

    Key type_to_remove_from = state_types.at(state_key).type_key;
    vector<int>& selected_type = type_buckets.at(type_to_remove_from);

    Key result_key = selected_type[0];
    swap(selected_type.front(), selected_type.back());
    selected_type.pop_back();
    adjust_heap_up(
        selected_type,
        selected_type.size()-1,
        &BTSInterBiasedIntraAnyOpenList<Entry>::compare_parent_node_smaller
    );

    --size;
    return state_types.at(result_key).entry;

}

template<class Entry>
BTSInterBiasedIntraAnyOpenList<Entry>::BTSInterBiasedIntraAnyOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      inter_evaluator(opts.get<shared_ptr<Evaluator>>("inter_eval")),
      intra_evaluator(opts.get<shared_ptr<Evaluator>>("intra_eval")), 
      tau(opts.get<double>("tau")),
      epsilon(opts.get<double>("epsilon")),
      size(0) {
}

template<class Entry>
bool BTSInterBiasedIntraAnyOpenList<Entry>::empty() const {
    return states.empty();
}

template<class Entry>
void BTSInterBiasedIntraAnyOpenList<Entry>::clear() {
    type_buckets.clear();
    states.clear();
    state_types.clear();
    size=0;
}

template<class Entry>
bool BTSInterBiasedIntraAnyOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(inter_evaluator.get());
}

template<class Entry>
bool BTSInterBiasedIntraAnyOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && inter_evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterBiasedIntraAnyOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    inter_evaluator->get_path_dependent_evaluators(evals);
    intra_evaluator->get_path_dependent_evaluators(evals);
}

BTSInterBiasedIntraAnyOpenListFactory::BTSInterBiasedIntraAnyOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterBiasedIntraAnyOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraAnyOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterBiasedIntraAnyOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraAnyOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterBiasedIntraAnyOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterBiasedIntraAnyOpenListFactory> {
public:
    BTSInterBiasedIntraAnyOpenListFeature() : TypedFeature("inter_bias_intra_any") {
        document_title("Type system to approximate bench transition system (BTS) and perform both inter- and intra-bench exploration");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "according to invasion percolation. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("inter_eval", "evaluator used for the type system");
        add_option<shared_ptr<Evaluator>>("intra_eval", "evaluator used for node order");
        add_option<double>("tau", "temperature parameter of softmin", "1.0");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<BTSInterBiasedIntraAnyOpenListFeature> _plugin;
}