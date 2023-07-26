#include "type_bts_intra_depth_open_list.h"

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

namespace type_intra_depth_open_list {
template<class Entry>
class BTSIntraDepthOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct TypeNode {
        int type_index;
        int h;
        int depth;
        Entry entry;
        TypeNode(int type_index, int h, int depth, const Entry &entry) 
            : type_index(type_index), h(h), depth(depth), entry(entry) {}
    };
    PerStateInformation<int> state_to_node_index;
    vector<vector<int>> type_heaps;
    vector<TypeNode> all_nodes;
    
    int cached_parent_depth;
    int cached_parent_h;
    int cached_parent_type_index;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool node_at_index_1_bigger(const int& index1, const int& index2);

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
    cached_parent_depth = -1;
    cached_parent_h = INT32_MAX;
    cached_parent_type_index = -1;
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    int cached_parent_index = state_to_node_index[parent_state];
    TypeNode parent = all_nodes[cached_parent_index];
    cached_parent_depth = parent.depth;
    cached_parent_h = parent.h;
    cached_parent_type_index = parent.type_index;
}

template<class Entry>
bool BTSIntraDepthOpenList<Entry>::node_at_index_1_bigger(const int& index1, const int& index2) {
    TypeNode first_node = all_nodes[index1];
    TypeNode second_node = all_nodes[index2];

    if (first_node.h == second_node.h) return first_node.depth < second_node.depth;
    return first_node.h > second_node.h;
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_index;
    int node_index = all_nodes.size(); 

    if (new_h < cached_parent_h) { 
        // if the new node is a new local minimum, it gets a new bucket
        type_index = type_heaps.size();
        type_heaps.push_back({node_index});
        TypeNode new_type_node(type_index, new_h, 0, entry);
        all_nodes.push_back(new_type_node);
    } else {
        // if the new node isn't a new local minimum, it gets bucketted with its parent
        type_index = cached_parent_type_index;
        type_heaps[type_index].push_back(node_index);
        TypeNode new_type_node(type_index, new_h, cached_parent_depth + 1, entry);
        all_nodes.push_back(new_type_node);
        auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
        {
            return node_at_index_1_bigger(elem1, elem2);
        };
        push_heap(type_heaps[type_index].begin(), type_heaps[type_index].end(), heap_compare);
    }
    state_to_node_index[eval_context.get_state()] = node_index;
}

template<class Entry>
Entry BTSIntraDepthOpenList<Entry>::remove_min() {
    int type_index;
    do {
        type_index = rng->random(type_heaps.size());
    } while (type_heaps[type_index].empty());
    vector<int> &type_heap = type_heaps[type_index];

    auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
    {
        return node_at_index_1_bigger(elem1, elem2);
    };
    pop_heap(type_heap.begin(), type_heap.end(), heap_compare);
    int node_index = type_heap.back();
    type_heap.pop_back();
    TypeNode min_node = all_nodes[node_index];

    return min_node.entry;
}

template<class Entry>
BTSIntraDepthOpenList<Entry>::BTSIntraDepthOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool BTSIntraDepthOpenList<Entry>::empty() const {
    return type_heaps.empty();
}

template<class Entry>
void BTSIntraDepthOpenList<Entry>::clear() {
    type_heaps.clear();
    all_nodes.clear();
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
    BTSIntraDepthOpenListFeature() : TypedFeature("bts_intra_depth") {
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