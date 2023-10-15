#ifndef SEARCH_ENGINES_MC_SEARCH_H
#define SEARCH_ENGINES_MC_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

#include <memory>
#include <vector>
#include <queue>

class Evaluator;

namespace plugins {
class Feature;
}

namespace mc_search {
class MonteCarloSearch : public SearchEngine {

    // const bool reopen_closed_nodes;
    std::shared_ptr<Evaluator> evaluator;
    double explore_param;

    struct MCTS_node {
        bool terminal;
        unsigned int num_sims;
        int score;                   
        std::vector<int> children_i;
        int parent_i;
        
        inline double uct(const double explore_param) const {
            double winrate = ((double) score) / ((double) num_sims);
            return winrate + explore_param * sqrt(std::log((double) num_sims) / ((double) num_sims));
        };

        inline int argmax_uct_of_children(const vector<MCTS_node> &monte_carlo_tree, const double explore_param) const {
            int max_i = 0;
            double max_uct = 0;
            for (int i: children_i) {
                if (monte_carlo_tree[i].uct(explore_param)) {

                }
            }
        };
    };
    vector<MCTS_node> monte_carlo_tree;

protected:

    virtual void initialize() override;
    MCTS_node& selection();
    void expansion(MCTS_node &to_expand);
    void backpropogation(MCTS_node &to_back_prop);
    virtual SearchStatus step() override;

public:
    explicit MonteCarloSearch(const plugins::Options &opts);
    virtual ~MonteCarloSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
