#include "lwm_inter_epsilon_intra_epsilon.h"

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

namespace lwm_inter_epsilon_intra_epsilon_open_list {
template<class Entry>
class LWMInterEpIntraEpOpenList : public OpenList<Entry> {
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
    PerStateInformation<int> state_type;
    vector<int> type_defs_index_heap;
    map<int, TypeDef> type_defs;
    
    int cached_parent_type_def_i;
    int cached_type_h;

    int last_removed_key;
    int last_removed_bucket_index;

    double inter_e;
    double intra_e;



protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool type_at_index_1_bigger(const int& index1, const int& index2);
    void adjust_type_up(vector<int> &type_heap, size_t pos);
    void adjust_node_up(vector<TypeNode> &heap, size_t pos);    

public:
    explicit LWMInterEpIntraEpOpenList(const plugins::Options &opts);
    virtual ~LWMInterEpIntraEpOpenList() override = default;

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
void LWMInterEpIntraEpOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_type_h = INT32_MAX;

    last_removed_bucket_index = -1;
    last_removed_key = -1;
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {

    cached_parent_type_def_i = state_type[parent_state];
    cached_type_h = type_defs[cached_parent_type_def_i].type_h;
}

template<class Entry>
bool LWMInterEpIntraEpOpenList<Entry>::type_at_index_1_bigger(const int& index1, const int& index2) {
    TypeDef first_type = type_defs[index1];
    TypeDef second_type = type_defs[index2];

    return first_type.type_h > second_type.type_h;
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_def_index;

    if (new_h < cached_type_h) { 

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
        TypeDef &cached_parent_type_def = type_defs[cached_parent_type_def_i];
        type_def_index = cached_parent_type_def_i;
        TypeNode new_node(new_h, entry);
        cached_parent_type_def.entries.push_back(new_node);  
        push_heap(
            cached_parent_type_def.entries.begin(), 
            cached_parent_type_def.entries.end(), 
            greater<TypeNode>());  
    }
    state_type[eval_context.get_state()] = type_def_index;
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::adjust_type_up(vector<int> &type_heap, size_t pos) {
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (type_at_index_1_bigger(type_heap[pos], type_heap[parent_pos])) {
            break;
        }
        swap(type_heap[pos], type_heap[parent_pos]);
        pos = parent_pos;
    }
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::adjust_node_up(vector<TypeNode> &heap, size_t pos) {
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
Entry LWMInterEpIntraEpOpenList<Entry>::remove_min() {
    int pos, type_i;
    
    // select a type
    while (true) {
        pos = 0;
        type_i = type_defs_index_heap[pos];

        if (rng->random() < inter_e) {
            pos = rng->random(type_defs_index_heap.size());
            type_i = type_defs_index_heap[pos];
        }

        if (type_defs[type_i].entries.empty()) {
            type_defs[type_i].type_h = numeric_limits<int>::min();
            adjust_type_up(type_defs_index_heap, pos);
            auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
            {
                return type_at_index_1_bigger(elem1, elem2);
            };
            pop_heap(
                type_defs_index_heap.begin(), 
                type_defs_index_heap.end(), 
                heap_compare);
            type_defs_index_heap.pop_back();
            type_defs.erase(type_i);
            continue;
        }
        break;
    }

    TypeDef &selected_type = type_defs[type_i];
    // select node in type
    pos = 0;
    if (rng->random() < intra_e) {
        pos = rng->random(selected_type.entries.size());
        selected_type.entries[pos].h = numeric_limits<int>::min();
        adjust_node_up(selected_type.entries, pos);
    }
    pop_heap(selected_type.entries.begin(), selected_type.entries.end(), greater<TypeNode>());
    TypeNode heap_node = selected_type.entries.back();
    selected_type.entries.pop_back();
    return heap_node.entry;

}

template<class Entry>
LWMInterEpIntraEpOpenList<Entry>::LWMInterEpIntraEpOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      inter_e(opts.get<double>("inter_e")),
      intra_e(opts.get<double>("intra_e")) {
}

template<class Entry>
bool LWMInterEpIntraEpOpenList<Entry>::empty() const {
    return type_defs.empty();
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::clear() {
    type_defs_index_heap.clear();
    type_defs.clear();
}

template<class Entry>
bool LWMInterEpIntraEpOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool LWMInterEpIntraEpOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void LWMInterEpIntraEpOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

LWMInterEpIntraEpOpenListFactory::LWMInterEpIntraEpOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LWMInterEpIntraEpOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LWMInterEpIntraEpOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LWMInterEpIntraEpOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LWMInterEpIntraEpOpenList<EdgeOpenListEntry>>(options);
}

class LWMInterEpIntraEpOpenListFeature : public plugins::TypedFeature<OpenListFactory, LWMInterEpIntraEpOpenListFactory> {
public:
    LWMInterEpIntraEpOpenListFeature() : TypedFeature("lwm_inter_ep_intra_ep") {
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

static plugins::FeaturePlugin<LWMInterEpIntraEpOpenListFeature> _plugin;
}