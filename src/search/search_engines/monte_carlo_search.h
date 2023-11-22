#ifndef SEARCH_ENGINES_MC_SEARCH_H
#define SEARCH_ENGINES_MC_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"
#include "../utils/rng.h"
#include "../heuristic.h"

#include <memory>
#include <vector>
#include <queue>

class Evaluator;

namespace plugins {
class Feature;
}

enum RolloutScores {LOSE = -1, TIE, WIN, SOLVE};

namespace mc_search {
class MonteCarloSearch : public SearchEngine {

    // const bool reopen_closed_nodes;
    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double explore_param;

    struct MC_RolloutAction {
        int edge_cost;
        OperatorID gen_op_id;
        StateID gen_state_id;
        MC_RolloutAction(int edge_cost, OperatorID gen_op_id, StateID gen_state_id) :
            edge_cost(edge_cost), gen_op_id(gen_op_id), gen_state_id(gen_state_id) {}
    };
    struct MCTS_State {
        int id;
        bool is_leaf;
        int eval;
        int num_sims;
        int num_wins;
        std::vector<StateID> children_ids;
        MCTS_State(
            int id,
            bool is_leaf,
            int eval,
            int num_sims,
            int num_wins
        ) : id(id), is_leaf(is_leaf), eval(eval), num_sims(num_sims), num_wins(num_wins) {}
        MCTS_State() {}
    };
    PerStateInformation<MCTS_State> mc_tree_data;
    StateID root_id = StateID::no_state;
    int next_mc_id = 0;

protected:
    inline bool is_dead_end(EvaluationContext &eval_context);
    virtual void initialize() override;
    StateID selection();
    SearchStatus child_expansion(StateID selected_id);
    void backpropogation(StateID to_back_prop_id);
    virtual SearchStatus step() override;
    int goal_count(const State &ancestor_state);

    inline double uct(State &state);
    inline StateID max_uct_child(std::vector<StateID> &children);

public:
    explicit MonteCarloSearch(const plugins::Options &opts);
    virtual ~MonteCarloSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
