#include "bts_inter_lwm_intra_epsilon.h"

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
    map<int, TypeDef> type_defs;

    int lwm_h;
    int lwm_node_typedef_i;

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
    void adjust_node_up(vector<TypeNode> &heap, size_t pos);    

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

    lwm_h = INT32_MAX;
    lwm_node_typedef_i = -1;

    last_removed_bucket_index = -1;
    last_removed_key = -1;
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {

    cached_parent_type_def_i = state_type[parent_state];
    cached_parent_h = state_h[parent_state];
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_def_index;

    if (new_h < cached_parent_h) { 

        TypeNode new_node(new_h, entry);
        TypeDef new_type_def(new_h, {new_node});
        type_def_index = eval_context.get_state().get_id().get_value();
        type_defs[type_def_index] = new_type_def;

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
    state_h[eval_context.get_state()] = new_h;
    state_type[eval_context.get_state()] = type_def_index;

    //update lwm value
    if (new_h < lwm_h) {
        lwm_h = new_h;
        lwm_node_typedef_i = type_def_index;
    }
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::adjust_node_up(vector<TypeNode> &heap, size_t pos) {
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
Entry BTSInterLWMIntraEpOpenList<Entry>::remove_min() {
    int pos, type_i;
    
    // select a type
    while (true) {

        if (lwm_node_typedef_i == -1 || rng->random() < inter_e) {
            auto it = type_defs.begin();
            std::advance(it,  rng->random(type_defs.size()) );
            type_i = it->first;
        } else {
            type_i = lwm_node_typedef_i;
        }

        if (type_defs[type_i].entries.empty()) {
            type_defs.erase(type_i);

            if (type_i == lwm_node_typedef_i) {
                lwm_node_typedef_i = -1;
            }

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
BTSInterLWMIntraEpOpenList<Entry>::BTSInterLWMIntraEpOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      inter_e(opts.get<double>("inter_e")),
      intra_e(opts.get<double>("intra_e")) {
}

template<class Entry>
bool BTSInterLWMIntraEpOpenList<Entry>::empty() const {
    return type_defs.empty();
}

template<class Entry>
void BTSInterLWMIntraEpOpenList<Entry>::clear() {
    type_defs.clear();
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
    BTSInterLWMIntraEpOpenListFeature() : TypedFeature("bts_inter_lwm_intra_ep") {
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

static plugins::FeaturePlugin<BTSInterLWMIntraEpOpenListFeature> _plugin;
}