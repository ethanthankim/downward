#include "monte_carlo_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"
#include "../utils/rng_options.h"

using namespace std;

namespace plugin_mc_greedy {
class MonteCarloSearchFeature : public plugins::TypedFeature<SearchEngine, mc_search::MonteCarloSearch> {
public:
    MonteCarloSearchFeature() : TypedFeature("mc_search") {
        document_title("Monte Carlo Search");
        document_synopsis("");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>("c", "exploration paramter", "1.414");
        mc_search::add_options_to_feature(*this);
        utils::add_rng_options(*this); 
    }
};

static plugins::FeaturePlugin<MonteCarloSearchFeature> _plugin;
}
