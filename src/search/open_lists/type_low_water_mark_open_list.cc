#include "type_low_water_mark_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"
#include "../search_engine.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;

namespace lwm_based_open_list {
template<class Entry>
class LWMBasedOpenList : public OpenList<Entry> {

    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;
    SearchSpace *search_space;

    struct LWMType {
        int h;
        vector<Entry> entries;
        LWMType(int h, const vector<Entry> &entries)
            : h(h), entries{std::move(entries)} {}
    };

    utils::HashMap<int, int> nodeid_to_type_index;
    utils::HashMap<int, LWMType> buckets;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;
    void add_to_type(int node_id, int type_index, const Entry &entry);
    void add_new_type(int node_id, int h, const Entry &entry);

public:
    explicit LWMBasedOpenList(const plugins::Options &opts);
    virtual ~LWMBasedOpenList() override = default;

    void set_search_space(SearchSpace &search_space);

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
};


template<class Entry>
void LWMBasedOpenList<Entry>::set_search_space(SearchSpace &search_space) {
    this->search_space = &search_space;
}

template<class Entry>
void LWMBasedOpenList<Entry>::add_new_type(int node_id, int h, const Entry &entry) {
    LWMType new_type(h, {entry});
    buckets[node_id] = new_type;
    nodeid_to_type_index[node_id] = node_id;
}

template<class Entry>
void LWMBasedOpenList<Entry>::add_to_type(int node_id, int type_index, const Entry &entry) {
    buckets[type_index].entries.push_back(entry);
    nodeid_to_type_index[node_id] = type_index;
}

template<class Entry>
void LWMBasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    int new_id = eval_context.get_state().get_id().get_value();
    int parent_id = this->search_space->get_node(eval_context.get_state()).get_info().parent_state_id.get_value();
    
    const StateRegistry *registry = eval_context.get_state().get_registry();
    const segmented_vector::SegmentedVector<Entry> *seg = PerStateInformation::get_entries(registry);
    // special case for a node that has no parent (i.e. the start node)
    if (parent_id == -1) {
        add_new_type(new_id, new_h, entry);
        return;
    }

    // get parent and type info
    int type_index = nodeid_to_type_index[parent_id];
    if (new_h < buckets[type_index].h) {
        add_new_type(new_id, new_h, entry);
    } else {
        add_to_type(new_id, type_index, entry);
    }
}

template<class Entry>
Entry LWMBasedOpenList<Entry>::remove_min() {
    size_t bucket_i = rng->random(buckets.size());
    LWMType &bucket = buckets[bucket_i];
    int pos = rng->random(bucket.entries.size());
    Entry result = utils::swap_and_pop_from_vector(bucket.entries, pos);

    if (bucket.entries.empty()) {
        buckets.erase(bucket_i);
    }
    return result;
}

template<class Entry>
LWMBasedOpenList<Entry>::LWMBasedOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
bool LWMBasedOpenList<Entry>::empty() const {
    return buckets.empty();
}

template<class Entry>
void LWMBasedOpenList<Entry>::clear() {
    buckets.clear();
    nodeid_to_type_index.clear();
}

template<class Entry>
bool LWMBasedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool LWMBasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void LWMBasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

LWMBasedOpenListFactory::LWMBasedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LWMBasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LWMBasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LWMBasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LWMBasedOpenList<EdgeOpenListEntry>>(options);
}

class LWMBasedOpenListFeature : public plugins::TypedFeature<OpenListFactory, LWMBasedOpenListFactory> {
public:
    LWMBasedOpenListFeature() : TypedFeature("lwm_based_type") {
        document_title("Low water mark based type system open list");
        document_synopsis(
            "Uses local search tree minima to assign entries to a bucket. "
            "All entries in a bucket are part of the same local minimum in the search tree."
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "uniformly randomly. "
            "TODO: add non-uniform type and node selection");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<LWMBasedOpenListFeature> _plugin;
}