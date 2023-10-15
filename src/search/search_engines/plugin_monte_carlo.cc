#include "monte_carlo_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_mc_greedy {
class MonteCarloSearchFeature : public plugins::TypedFeature<SearchEngine, mc_search::MonteCarloSearch> {
public:
    MonteCarloSearchFeature() : TypedFeature("eager_greedy") {
        document_title("Greedy search (eager)");
        document_synopsis("");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>("c", "exploration paramter");
        mc_search::add_options_to_feature(*this);

        document_note(
            "Something",
            "something else"); 
    }

    virtual shared_ptr<mc_search::MonteCarloSearch> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(context, options, "evals");
        plugins::Options options_copy(options);
        options_copy.set("open", search_common::create_greedy_open_list_factory(options_copy));
        options_copy.set("reopen_closed", false);
        shared_ptr<Evaluator> evaluator = nullptr;
        options_copy.set("f_eval", evaluator);

        return make_shared<mc_search::MonteCarloSearch>(options_copy);
    }
};

static plugins::FeaturePlugin<MonteCarloSearchFeature> _plugin;
}
