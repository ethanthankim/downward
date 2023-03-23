#include "type_low_water_mark_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

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
    vector<shared_ptr<Evaluator>> evaluators;

    using Key = vector<int>;
    using Bucket = vector<Entry>;
    vector<pair<Key, Bucket>> keys_and_buckets;
    utils::HashMap<Key, int> key_to_bucket_index;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit LWMBasedOpenList(const plugins::Options &opts);
    virtual ~LWMBasedOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
};

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

        add_list_option<shared_ptr<Evaluator>>(
            "evaluators",
            "Evaluators used to determine the bucket for each entry.");
        utils::add_rng_options(*this);
    }

    virtual shared_ptr<LWMBasedOpenListFactory> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, options, "evaluators");
        return make_shared<LWMBasedOpenListFactory>(options);
    }
};

static plugins::FeaturePlugin<LWMBasedOpenListFeature> _plugin;
}