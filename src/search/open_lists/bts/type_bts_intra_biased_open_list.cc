#include "type_bts_intra_biased_open_list.h"

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

namespace type_intra_biased_open_list {
template<class Entry>
class BTSIntraBiasedOpenList : public OpenList<Entry> {
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
    vector<map<int, deque<Entry>>> type_buckets;
    vector<TypeNode> all_nodes;
    
    int cached_parent_depth;
    int cached_parent_h;
    int cached_parent_type_index;

    double alpha;
    double beta;
    double epsilon;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool node_at_index_1_bigger(const int& index1, const int& index2);

public:
    explicit BTSIntraBiasedOpenList(const plugins::Options &opts);
    virtual ~BTSIntraBiasedOpenList() override = default;

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
void BTSIntraBiasedOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_depth = -1;
    cached_parent_h = INT32_MAX;
    cached_parent_type_index = -1;
}

template<class Entry>
void BTSIntraBiasedOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    int cached_parent_index = state_to_node_index[parent_state];
    TypeNode parent = all_nodes[cached_parent_index];
    cached_parent_depth = parent.depth;
    cached_parent_h = parent.h;
    cached_parent_type_index = parent.type_index;
}

template<class Entry>
bool BTSIntraBiasedOpenList<Entry>::node_at_index_1_bigger(const int& index1, const int& index2) {
    TypeNode first_node = all_nodes[index1];
    TypeNode second_node = all_nodes[index2];

    if (first_node.h == second_node.h) return first_node.depth < second_node.depth;
    return first_node.h > second_node.h;
}

template<class Entry>
void BTSIntraBiasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_index;
    int node_index = all_nodes.size();

    if (new_h < cached_parent_h) { 

        type_index = type_buckets.size();
        map<int, deque<Entry>> new_type; 
        type_buckets.push_back(new_type);
        type_buckets[type_index][new_h].push_back(entry);

        TypeNode new_type_node(type_index, new_h, 0, entry);
        all_nodes.push_back(new_type_node);
    } else {
        type_index = cached_parent_type_index;
        type_buckets[type_index][new_h].push_back(entry);
        TypeNode new_type_node(type_index, new_h, cached_parent_depth + 1, entry);
        all_nodes.push_back(new_type_node);
    }
    state_to_node_index[eval_context.get_state()] = node_index;
}

template<class Entry>
Entry BTSIntraBiasedOpenList<Entry>::remove_min() {
    int type_index;
    do {
        type_index = rng->random(type_buckets.size());
    } while (type_buckets[type_index].empty());
    map<int, deque<Entry>> &type_bucket = type_buckets[type_index];
    int key = type_bucket.begin()->first;

    if (type_bucket.size() > 1) {
        double r = rng->random();
        if (r <= epsilon) {
            double total_sum = 0;
            double bias = beta + alpha * static_cast<double>(type_bucket.rbegin()->first);
            for (auto it : type_bucket) {
                double s = -1.0 * alpha * static_cast<double>(it.first) + bias;
                total_sum += s;
            }
            double p_sum = 0.0;
            for (auto it : type_bucket) {
                double p = (-1.0 * alpha * static_cast<double>(it.first) + bias) / total_sum;
                p_sum += p;
                if (r <= p_sum * epsilon) {
                    key = it.first;
                    break;
                }
            }
        }
    }

    deque<Entry> &bucket = type_bucket[key];
    assert(!bucket.empty());
    Entry result = bucket.front();
    bucket.pop_front();
    if (bucket.empty()) type_bucket.erase(key);
    return result;
}

template<class Entry>
BTSIntraBiasedOpenList<Entry>::BTSIntraBiasedOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      epsilon(opts.get<double>("epsilon")) {
}

template<class Entry>
bool BTSIntraBiasedOpenList<Entry>::empty() const {
    return type_buckets.empty();
}

template<class Entry>
void BTSIntraBiasedOpenList<Entry>::clear() {
    type_buckets.clear();
    all_nodes.clear();
}

template<class Entry>
bool BTSIntraBiasedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSIntraBiasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSIntraBiasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSIntraBiasedOpenListFactory::BTSIntraBiasedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSIntraBiasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSIntraBiasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSIntraBiasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSIntraBiasedOpenList<EdgeOpenListEntry>>(options);
}

class BTSIntraBiasedOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSIntraBiasedOpenListFactory> {
public:
    BTSIntraBiasedOpenListFeature() : TypedFeature("bts_intra_biased") {
        document_title("Type system to approximate bench transition system (BTS) and perform both inter- and intra-bench exploration");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "according to invasion percolation. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>("alpha", "coefficent", "1.0");
        add_option<double>("beta", "bias", "1.0");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<BTSIntraBiasedOpenListFeature> _plugin;
}