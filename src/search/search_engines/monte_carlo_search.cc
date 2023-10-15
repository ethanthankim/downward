#include "monte_carlo_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../open_list_factory.h"
#include "../pruning_method.h"

#include "../algorithms/ordered_set.h"
#include "../plugins/options.h"
#include "../task_utils/successor_generator.h"
#include "../utils/logging.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional.hh>
#include <set>

using namespace std;

namespace mc_search {
MonteCarloSearch::MonteCarloSearch(const plugins::Options &opts)
    : SearchEngine(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    explore_param(opts.get<double>("c"))
{}

void MonteCarloSearch::initialize() {

    State initial_state = state_registry.get_initial_state();
    EvaluationContext eval_context(initial_state, 0, true, &statistics);
    statistics.inc_evaluated_states();

    if (search_progress.check_progress(eval_context))
        statistics.print_checkpoint_line(0);

    SearchNode node = search_space.get_node(initial_state);
    node.open_initial();
    print_initial_evaluator_values(eval_context);
}

void MonteCarloSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}

MonteCarloSearch::MCTS_node& MonteCarloSearch::selection() {

    int curr_i = 0;
    auto &current = monte_carlo_tree[curr_i];

    while(!current.terminal) {
        current = monte_carlo_tree[
            current.argmax_uct_of_children(monte_carlo_tree, explore_param)
        ];
    }

    return current;

}

void MonteCarloSearch::expansion(MCTS_node &to_expand) {

}

void MonteCarloSearch::backpropogation(MCTS_node &to_back_prop) {

}

SearchStatus MonteCarloSearch::step() {
    
    auto& selected = selection();
    expansion(selected);
    backpropogation(selected);

}

void MonteCarloSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void add_options_to_feature(plugins::Feature &feature) {
    SearchEngine::add_options_to_feature(feature);
}
}
