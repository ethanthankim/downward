#include "type_bts_inter_biased_open_list.h"

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

namespace type_inter_biased_open_list {
template<class Entry>
class BTSInterBiasedOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct Bucket {
        int type_def_i;
        vector<Entry> entries;
        Bucket(int type_def_i, const vector<Entry> &entries) 
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
    map<int, vector<Bucket>> type_buckets;
    vector<TypeDef> type_defs;
    
    TypeDef cached_parent_type;
    int cached_parent_h;

    double alpha;
    double beta;
    double epsilon;


protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit BTSInterBiasedOpenList(const plugins::Options &opts);
    virtual ~BTSInterBiasedOpenList() override = default;

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
void BTSInterBiasedOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_parent_h = INT32_MAX;
    cached_parent_type.bucket_index = -1;
    cached_parent_type.type_h = -1;
}

template<class Entry>
void BTSInterBiasedOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    int parent_type_index = state_type[parent_state];
    cached_parent_type = type_defs[parent_type_index];
    cached_parent_h = state_h[parent_state];
}

template<class Entry>
void BTSInterBiasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int type_def_index;

    if (new_h < cached_parent_h) { 

        int bucket_index = type_buckets[new_h].size();
        type_def_index = type_defs.size();
        TypeDef new_type_def(bucket_index, new_h);
        type_defs.push_back(new_type_def);
        
        Bucket new_bucket(type_def_index, {entry});
        type_buckets[new_h].push_back(new_bucket);

    } else {
        int bucket_index = cached_parent_type.bucket_index;
        type_def_index = type_buckets[cached_parent_type.type_h][bucket_index].type_def_i;
        type_buckets[cached_parent_type.type_h][bucket_index].entries.push_back(entry);
    }
    state_h[eval_context.get_state()] = new_h;
    state_type[eval_context.get_state()] = type_def_index;
}

template<class Entry>
Entry BTSInterBiasedOpenList<Entry>::remove_min() {

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

        vector<Bucket> &buckets = type_buckets[key];
        if (buckets.empty()) {

            type_buckets.erase(key);
            continue;
        }

        int bucket_index = rng->random(buckets.size());
        Bucket &bucket = type_buckets[key][bucket_index];
        if (bucket.entries.empty()) {
            
            utils::swap_and_pop_from_vector(buckets, bucket_index);
            if (bucket_index < buckets.size())
                type_defs[buckets[bucket_index].type_def_i].bucket_index = bucket_index;

            utils::swap_and_pop_from_vector(type_defs, bucket.type_def_i);
            if (bucket.type_def_i < type_defs.size()) {
                TypeDef tmp = type_defs[bucket.type_def_i];
                type_buckets[tmp.type_h][tmp.bucket_index].type_def_i = bucket.type_def_i;
            }


            continue;
        }

        int entry_index = rng->random(bucket.entries.size());
        Entry result = utils::swap_and_pop_from_vector(bucket.entries, entry_index);
        return result;

    }
}

template<class Entry>
BTSInterBiasedOpenList<Entry>::BTSInterBiasedOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")), 
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      epsilon(opts.get<double>("epsilon")) {
}

template<class Entry>
bool BTSInterBiasedOpenList<Entry>::empty() const {
    return type_buckets.empty();
}

template<class Entry>
void BTSInterBiasedOpenList<Entry>::clear() {
    type_buckets.clear();
}

template<class Entry>
bool BTSInterBiasedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool BTSInterBiasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void BTSInterBiasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

BTSInterBiasedOpenListFactory::BTSInterBiasedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
BTSInterBiasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
BTSInterBiasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<BTSInterBiasedOpenList<EdgeOpenListEntry>>(options);
}

class BTSInterBiasedOpenListFeature : public plugins::TypedFeature<OpenListFactory, BTSInterBiasedOpenListFactory> {
public:
    BTSInterBiasedOpenListFeature() : TypedFeature("bts_inter_biased") {
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

static plugins::FeaturePlugin<BTSInterBiasedOpenListFeature> _plugin;
}