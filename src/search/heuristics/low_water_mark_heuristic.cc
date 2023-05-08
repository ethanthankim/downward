#include "low_water_mark_heuristic.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <cassert>
#include <vector>

using namespace std;

namespace low_water_mark_heuristic {

// construction and destruction
LowWaterMarkHeuristic::LowWaterMarkHeuristic(const plugins::Options &opts)
    : RelaxationHeuristic(opts) {
    if (log.is_at_least_normal()) {
        log << "Initializing low water mark heuristic..." << endl;
    }
}

int LowWaterMarkHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);

    int cost = 0;
    state.get_unpacked_values()
    auto t = state.get_registry()->lookup_state(state.get_id());
    return cost;
}

class LowWaterMarkHeuristicFeature : public plugins::TypedFeature<Evaluator, LowWaterMarkHeuristic> {
public:
    LowWaterMarkHeuristicFeature() : TypedFeature("lwm") {
        document_title("Low water mark heuristic");

        Heuristic::add_options_to_feature(*this);

        document_language_support("action costs", "supported");
        /*
        TODO: IDK what to put here
        */
    }
};

static plugins::FeaturePlugin<LowWaterMarkHeuristicFeature> _plugin;
}
