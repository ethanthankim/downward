#include "random_edge_evaluator.h"

#include "../evaluation_context.h"
#include "../search_space.h"
#include "../plugins/plugin.h"
#include "../../utils/rng_options.h"

#include <vector>
#include <cassert>
using namespace std;

namespace RandomEdgeEvaluator {
    RandomEdgeEvaluator::RandomEdgeEvaluator(const plugins::Options &opts) :
        Evaluator(opts), state_db(-1),
        bound((int)(numeric_limits<int>::max() * opts.get<double>("threshold"))) {}
    
    EvaluationResult RandomEdgeEvaluator::compute_result(EvaluationContext &ctx) {
        EvaluationResult result;
        auto current = ctx.get_state();
        int &state_value = state_db[current];
        if (state_value < 0){
            state_value = rng->random(std::numeric_limits<int>::max());
        }
        auto op = ctx.get_state().get_task().get_operators()[0];

        auto it = edge_db.find(&op); 
        int edge_value;
        if (it == edge_db.end()){
            edge_value = rng->random(std::numeric_limits<int>::max());
            edge_db[&op] = edge_value;
        }else{
            edge_value = it->second;
        }

        int value = abs(state_value ^ edge_value);
        
        if (value > bound){
            value = EvaluationResult::INFTY;
        }
        
        result.set_evaluator_value(value);
        return result;
    }


class RandomEdgeEvaluatorFeature : public plugins::TypedFeature<Evaluator, RandomEdgeEvaluator> {
public:
    RandomEdgeEvaluatorFeature() : TypedFeature("random_edge") {
        document_subcategory("evaluators_basic");
        document_title("Weighted evaluator");
        document_synopsis(
            "RandomEdge evaluator. Calculates the random value for a node, based on the edge it comes from.");

        add_option<double>(
            "threshold",
            "Threshold for random value. Mapped into 32bit integer space and "
            "the random value above the threshold is treated as infinite (pruned). "
            "Any value below 1.0 makes the algorithm incomplete.",
            "1.0",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_evaluator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<RandomEdgeEvaluatorFeature> _plugin;
}
