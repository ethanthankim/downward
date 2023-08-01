#include "bts_inter_biased_intra_percolation.h"

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

namespace inter_biased_intra_percolation_open_list {
template<class Entry>
class BTSInterBiasedIntraPercolationOpenList : public OpenList<Entry> {
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
    struct TypeBucket {
        int type_def_i;
        vector<TypeNode> entries;
        TypeBucket(int type_def_i, const vector<TypeNode> &entries) 
            : type_def_i(type_def_i), entries(entries) {}
    };
    struct TypeDef {
        int bucket_index;
        int type_h;
        TypeDef(int bucket_index, int type_h) 
            : bucket_index(bucket_index), type_h(type_h) {};
        TypeDef() : bucket_index(-1), type_h(-1) {}
    };
    PerStateInformation<int> state_h;
    PerStateInformation<int> state_type;
    map<int, vector<TypeBucket>> type_buckets;
    map<int, TypeDef> type_defs;
    
    int cached_parent_type_bucket_index;
    int cached_parent_type_h;

    int cached_parent_h;
    int last_removed_key;
    int last_removed_bucket_index;

    double alpha;
    double beta;
    double epsilon;



protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

// private:
//     bool node_at_index_1_bigger(const int& index1, const int& index2);

public:
    explicit BTSInterBiasedIntraPercolationOpenList(const plugins::Options &opts);
    virtual ~BTSInterBiasedIntraPercolationOpenList() override = default;

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
void BTSInterBiasedIntraPercolationOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
    cached_parent_type_bucket_index = -1;
    cached_parent_type_h = -1;

    last_removed_bucket_index = -1;
    last_removed_key = -1;
}

template<class Entry>
void BTSInterBiasedIntraPercolationOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    int parent_type_i = state_type[parent_state];
    TypeDef parent_type = type_defs[parent_type_i];
    cached_parent_type_bucket_index = parent_type.bucket_index;
    cached_parent_type_h = parent_type.type_h;
    cached_parent_h = state_h[parent_state];
}

// template<class Entry>
// bool BTSInterBiasedIntraHeapOpenList<Entry>::node_at_index_1_bigger(const int& index1, const int& index2) {
//     TypeNode first_node = all_nodes[index1];
//     TypeNode second_node = all_nodes[index2];

//     if (first_node.h == second_node.h) return first_node.depth < second_node.depth;
//     return first_node.h > second_node.h;
// }

template<class Entry>
void BTSInterBiasedIntraPercolationOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_def_index;
    int rand_h = rng->random(new_h > 0 ? new_h : 1);

    if (new_h < cached_parent_h) { 

        int bucket_index = type_buckets[new_h].size();
        type_def_index = eval_context.get_state().get_id().get_value();

        TypeDef new_type_def(bucket_index, new_h);
        type_defs[type_def_index] = new_type_def;
        
        TypeNode new_node(rand_h, entry);
        TypeBucket new_bucket(type_def_index, {new_node});
        type_buckets[new_h].push_back(new_bucket);

    } else {
        int bucket_index = cached_parent_type_bucket_index;
        type_def_index = type_buckets[cached_parent_type_h][bucket_index].type_def_i;
        TypeNode new_node(rand_h, entry);
        type_buckets[cached_parent_type_h][bucket_index].entries.push_back(new_node);  
        push_heap(
            type_buckets[cached_parent_type_h][bucket_index].entries.begin(), 
            type_buckets[cached_parent_type_h][bucket_index].entries.end(), 
            greater<TypeNode>());  
    }
    state_h[eval_context.get_state()] = new_h;
    state_type[eval_context.get_state()] = type_def_index;
}

template<class Entry>
Entry BTSInterBiasedIntraPercolationOpenList<Entry>::remove_min() {

    if (!(last_removed_key == -1)) {

        vector<TypeBucket> &buckets = type_buckets[last_removed_key];
        TypeBucket bucket = buckets[last_removed_bucket_index];
        if (bucket.entries.empty()) {
            utils::swap_and_pop_from_vector(buckets, last_removed_bucket_index);
            if (last_removed_bucket_index < buckets.size())
                type_defs[buckets[last_removed_bucket_index].type_def_i].bucket_index = last_removed_bucket_index;
            
            type_defs.erase(bucket.type_def_i);
            if (buckets.empty())
                type_buckets.erase(last_removed_key);
        }
    }

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

        vector<TypeBucket> &buckets = type_buckets[key];
        int bucket_index = rng->random(buckets.size());
        TypeBucket &bucket = type_buckets[key][bucket_index];

        last_removed_bucket_index = bucket_index;
        last_removed_key = key;

        pop_heap(bucket.entries.begin(), bucket.entries.end(), greater<TypeNode>());
        TypeNode heap_node = bucket.entries.back();
        bucket.entries.pop_back();
        return heap_node.entry;

    }
}

template<class Entry>
BTSInterBiasedIntraPercolationOpenList<Entry>::BTSInterBiasedIntraPercolationOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      epsilon(opts.get<double>("epsilon")) {
}

template<class Entry>
bool BTSInterBiasedIntraPercolationOpenList<Entry>::empty() const {
    return type_buckets.empty();
}

template<class Entry>
void BTSInterBiasedIntraPercolationOpenList<Entry>::clear() {
    type_buckets.clear();
}

template<class Entry>
bool BTSInterBiasedIntraPercolationOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterBiasedIntraPercolationOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterBiasedIntraPercolationOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterBiasedIntraPercolationOpenListFactory::BTSInterBiasedIntraPercolationOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterBiasedIntraPercolationOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraPercolationOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterBiasedIntraPercolationOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedIntraPercolationOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterBiasedIntraPercolationOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterBiasedIntraPercolationOpenListFactory> {
public:
    BTSInterBiasedIntraPercolationOpenListFeature() : TypedFeature("bts_inter_biased_intra_percolation") {
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

static plugins::FeaturePlugin<BTSInterBiasedIntraPercolationOpenListFeature> _plugin;
}