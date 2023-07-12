#include "bts_inter_epsilon_intra_epsilon.h"

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

namespace inter_epsilon_intra_epsilon_open_list {
template<class Entry>
class BTSInterEpIntraEpOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct TypeNode {
        int h;
        Entry entry;
        TypeNode(int h, const Entry &entry) 
            : h(h), entry(entry) {}
        
        bool operator>(const TypeNode &other) const {
            return h > other.h;
        }
    };
    struct TypeDef {
        int type_h;
        vector<TypeNode> entries;
        TypeDef(int type_h, const vector<TypeNode> &entries) 
            : type_h(type_h), entries(entries) {};
        TypeDef() : type_h(-1), entries({}){}
    };
    PerStateInformation<int> state_h;
    PerStateInformation<int> state_type;
    vector<int> type_defs_index_heap;
    map<int, TypeDef> type_defs;
    
    TypeDef cached_parent_type_def;
    int cached_parent_type_def_i;
    int cached_parent_h;

    int last_removed_key;
    int last_removed_bucket_index;

    double inter_e;
    double intra_e;



protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool type_at_index_1_bigger(const int& index1, const int& index2);

public:
    explicit BTSInterEpIntraEpOpenList(const plugins::Options &opts);
    virtual ~BTSInterEpIntraEpOpenList() override = default;

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
void BTSInterEpIntraEpOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;

    last_removed_bucket_index = -1;
    last_removed_key = -1;
}

template<class Entry>
void BTSInterEpIntraEpOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {

    cached_parent_type_def_i = state_type[parent_state];
    cached_parent_type_def = type_defs[cached_parent_type_def_i];
    cached_parent_h = state_h[parent_state];
}

template<class Entry>
bool BTSInterEpIntraEpOpenList<Entry>::type_at_index_1_bigger(const int& index1, const int& index2) {
    TypeDef first_type = type_defs[index1];
    TypeDef second_type = type_defs[index2];

    return first_type.type_h > second_type.type_h;
}

template<class Entry>
void BTSInterEpIntraEpOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_def_index;

    if (new_h < cached_parent_h) { 

        TypeNode new_node(new_h, entry);
        TypeDef new_type_def(new_h, {new_node});
        type_def_index = eval_context.get_state().get_id().get_value();
        type_defs[type_def_index] = new_type_def;

        type_defs_index_heap.push_back(type_def_index);
        auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
        {
            return type_at_index_1_bigger(elem1, elem2);
        };
        push_heap(
            type_defs_index_heap.begin(), 
            type_defs_index_heap.end(), 
            heap_compare); 

    } else {
        type_def_index = cached_parent_type_def_i;
        TypeNode new_node(new_h, entry);
        cached_parent_type_def.entries.push_back(new_node);  
        push_heap(
            cached_parent_type_def.entries.begin(), 
            cached_parent_type_def.entries.end(), 
            greater<TypeNode>());  
    }
    state_h[eval_context.get_state()] = new_h;
    state_type[eval_context.get_state()] = type_def_index;
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
Entry BTSInterEpIntraEpOpenList<Entry>::remove_min() {

    // select a type
    int type_i = 0;
    int pos = 0;
    TypeDef select_type;
    while (true) {
        if (rng->random() < inter_e) {
            pos = rng->random(type_defs_index_heap.size());
            type_i = type_defs_index_heap[pos];
        }

        if (type_defs[type_i].entries.empty()) {
            auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
            {
                return type_at_index_1_bigger(elem1, elem2);
            };
            pop_heap(
                type_defs_index_heap.begin() + pos, 
                type_defs_index_heap.end(), 
                heap_compare);
            type_defs_index_heap.pop_back();
            type_defs.erase(type_i);
            continue;
        } else 
            select_type = type_defs[type_i];

        break;
    }

    // select node in type
    pos = 0;
    if (rng->random() < intra_e) {
        pos = rng->random(select_type.entries.size());
    }
    pop_heap(select_type.entries.begin() + pos, select_type.entries.end(), greater<TypeNode>());
    TypeNode heap_node = select_type.entries.back();
    select_type.entries.pop_back();
    return heap_node.entry;

}

template<class Entry>
BTSInterEpIntraEpOpenList<Entry>::BTSInterEpIntraEpOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      inter_e(opts.get<double>("inter_e")),
      intra_e(opts.get<double>("intra_e")) {
}

template<class Entry>
bool BTSInterEpIntraEpOpenList<Entry>::empty() const {
    return type_defs.empty();
}

template<class Entry>
void BTSInterEpIntraEpOpenList<Entry>::clear() {
    type_defs_index_heap.clear();
    type_defs.clear();
}

template<class Entry>
bool BTSInterEpIntraEpOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterEpIntraEpOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterEpIntraEpOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterEpIntraEpOpenListFactory::BTSInterEpIntraEpOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterEpIntraEpOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterEpIntraEpOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterEpIntraEpOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterEpIntraEpOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterEpIntraEpOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterEpIntraEpOpenListFactory> {
public:
    BTSInterEpIntraEpOpenListFeature() : TypedFeature("bts_inter_ep_intra_ep") {
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

static plugins::FeaturePlugin<BTSInterEpIntraEpOpenListFeature> _plugin;
}