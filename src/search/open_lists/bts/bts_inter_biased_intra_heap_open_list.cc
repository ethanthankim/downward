#include "bts_inter_biased_intra_heap_open_list.h"

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

namespace inter_biased_intra_heap_open_list {
template<class Entry>
class BTSInterBiasedIntraHeapOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct TypeNode {
        int bucket_index;
        int type_h;
        int h;
        Entry entry;
        TypeNode(int bucket_index, int type_h, int h, const Entry &entry) 
            : bucket_index(bucket_index), type_h(type_h), h(h), entry(entry) {}
    };
    PerStateInformation<int> state_to_node_index;
    map<int, vector<vector<int>>> type_buckets;
    vector<TypeNode> all_nodes;
    
    int cached_type_h;
    int cached_parent_h;
    int cached_parent_bucket_index;

    double alpha;
    double beta;
    double epsilon;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    bool node_at_index_1_bigger(const int& index1, const int& index2);

public:
    explicit BTSInterBiasedIntraHeapOpenList(const plugins::Options &opts);
    virtual ~BTSInterBiasedIntraHeapOpenList() override = default;

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
void BTSInterBiasedIntraHeapOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_type_h = -1;
    cached_parent_h = INT32_MAX;
    cached_parent_bucket_index = -1;
}

template<class Entry>
void BTSInterBiasedIntraHeapOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    int cached_parent_index = state_to_node_index[parent_state];
    TypeNode parent = all_nodes[cached_parent_index];
    cached_parent_h = parent.h;
    cached_parent_bucket_index = parent.bucket_index;
    cached_type_h = parent.type_h;
}

template<class Entry>
bool BTSInterBiasedIntraHeapOpenList<Entry>::node_at_index_1_bigger(const int& index1, const int& index2) {
    TypeNode first_node = all_nodes[index1];
    TypeNode second_node = all_nodes[index2];

    // if (first_node.h == second_node.h) return first_node.depth < second_node.depth;
    return first_node.h > second_node.h;
}

template<class Entry>
void BTSInterBiasedIntraHeapOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_index;
    int node_index = all_nodes.size();

    if (new_h < cached_parent_h) { 

        type_index = type_buckets[new_h].size();
        type_buckets[new_h].push_back({node_index});

        TypeNode new_type_node(type_index, new_h, new_h, entry);
        all_nodes.push_back(new_type_node);

    } else {
        type_index = cached_parent_bucket_index;
        type_buckets[cached_type_h][type_index].push_back(node_index);
        TypeNode new_type_node(type_index, cached_type_h, new_h, entry);
        all_nodes.push_back(new_type_node);
        auto heap_compare = [&] (const int& elem1, const int& elem2) -> bool
        {
            return node_at_index_1_bigger(elem1, elem2);
        };
        push_heap(type_buckets[cached_type_h][type_index].begin(), type_buckets[cached_type_h][type_index].end(), heap_compare);
    }
    state_to_node_index[eval_context.get_state()] = node_index;
}

template<class Entry>
Entry BTSInterBiasedIntraHeapOpenList<Entry>::remove_min() {

    while(true) {

        int key = type_buckets.begin()->first;
        if (type_buckets.size() > 1) {
            double r = rng->random();
            if (r <= epsilon) {
                double total_sum = 0;
                double bias = beta + alpha * static_cast<double>(type_buckets.rbegin()->first);
                for (auto it : type_buckets) {
                    double s = -1.0 * alpha * static_cast<double>(it.first) + bias;
                    total_sum += s;
                }
                double p_sum = 0.0;
                for (auto it : type_buckets) {
                    double p = (-1.0 * alpha * static_cast<double>(it.first) + bias) / total_sum;
                    p_sum += p;
                    if (r <= p_sum * epsilon) {
                        key = it.first;
                        break;
                    }
                }
            }
        }

        vector<vector<Entry>> &buckets = type_buckets[key];
        if (buckets.empty()) {
            type_buckets.erase(key);
            continue;
        }

        int type_index = rng->random(buckets.size());
        vector<Entry> &bucket = type_buckets[key][type_index];
        if (bucket.empty()) {
            utils::swap_and_pop_from_vector(buckets, type_index);
            continue;
        }

        int entry_index = rng->random(bucket.size());
        Entry result = utils::swap_and_pop_from_vector(bucket, entry_index);
        return result;

    }
}

template<class Entry>
BTSInterBiasedIntraHeapOpenList<Entry>::BTSInterBiasedIntraHeapOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      epsilon(opts.get<double>("epsilon")) {
}

template<class Entry>
bool BTSInterBiasedIntraHeapOpenList<Entry>::empty() const {
    return type_buckets.empty();
}

template<class Entry>
void BTSInterBiasedIntraHeapOpenList<Entry>::clear() {
    type_buckets.clear();
    all_nodes.clear();
}

template<class Entry>
bool BTSInterBiasedIntraHeapOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterBiasedIntraHeapOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterBiasedIntraHeapOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterBiasedIntraHeapOpenListFactory::BTSInterBiasedIntraHeapOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterBiasedIntraHeapOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraHeapOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterBiasedIntraHeapOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraHeapOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterBiasedIntraHeapOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterBiasedIntraHeapOpenListFactory> {
public:
    BTSInterBiasedIntraHeapOpenListFeature() : TypedFeature("bts_inter_biased_intra_heap") {
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

static plugins::FeaturePlugin<BTSInterBiasedIntraHeapOpenListFeature> _plugin;
}