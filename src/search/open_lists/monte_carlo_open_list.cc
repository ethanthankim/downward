#include "monte_carlo_open_list.h"

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
#include <bits/stdc++.h>

using namespace std;

namespace monte_carlo_open_list {
template<class Entry>
class MonteCarloOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    struct Node {
        int eval;
        int visited;
        Entry entry; 
        bool is_leaf;
        Node* parent;
        std::vector<Node*> children;
        Node() {}
        Node(int eval, int visited, Entry entry, bool is_leaf, Node* parent) :
            eval(eval), visited(visited), entry(entry), is_leaf(is_leaf), parent(parent) {}
    };
    PerStateInformation<Node *> states_to_nodes;
    Node * cached_parent_node;
    Node * cached_initial_node;
    int c;
    
protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

private:
    Node& select();
    void backpropogate();
    double uct(Node &node);
    Node * max_utc_child(vector<Node *> &children);
    int min_h_child(vector<Node *> &children);

public:
    explicit MonteCarloOpenList(const plugins::Options &opts);
    virtual ~MonteCarloOpenList() override = default;

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
void MonteCarloOpenList<Entry>::notify_initial_state(const State &initial_state) {
    cached_initial_node = states_to_nodes[initial_state];
}

template<class Entry>
int MonteCarloOpenList<Entry>::min_h_child(vector<Node *> &children) {
    int min = children[0]->eval;
    for (typename vector<Node *>::iterator it = std::next(children.begin()); it != children.end(); it++) {
        if ((*it)->eval < min) {
            min = (*it)->eval;
        }
    }
    return min;
}

template<class Entry>
void MonteCarloOpenList<Entry>::backpropogate() {

    Node *start = cached_parent_node;
    while (start != cached_initial_node) {
        start->eval = min_h_child(start->children);
        start = start->parent;
    }
}

template<class Entry>
void MonteCarloOpenList<Entry>::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {

    // if the next expansion started, backpropogate previous expansion results
    if (cached_parent_node != states_to_nodes[parent_state]) 
        backpropogate();
    
    cached_parent_node = states_to_nodes[parent_state];
}

template<class Entry>
void MonteCarloOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(evaluator.get());
    double eval = (double) (1/new_h);
    cached_parent_node->children.push_back(new Node((int) eval*10, 0, entry, true, cached_parent_node));
    states_to_nodes[eval_context.get_state()] = cached_parent_node->children.back();

}

template<class Entry>
double MonteCarloOpenList<Entry>::uct(Node &node) {
    double exploitation = (double) (node.eval / node.visited);
    double exploration = c*sqrt(log(cached_initial_node->visited) / node.visited);
    return exploitation + exploration;
}

template<class Entry>
MonteCarloOpenList<Entry>::Node * MonteCarloOpenList<Entry>::max_utc_child(vector<Node *> &children) {
    Node * max_node = children[0];
    double max = uct(*max_node);
    for (typename vector<Node *>::iterator it = std::next(children.begin()); it != children.end(); it++) {
        if ((*it)->visited == 0) return *it; // if a node has never been visited, uct will be infinity
        
        double node_utc = uct(**it);
        if (node_utc > max) {
            max_node = *it;
            max = node_utc;
        }
    }
    return max_node;
}

template<class Entry>
MonteCarloOpenList<Entry>::Node& MonteCarloOpenList<Entry>::select() {
    cached_initial_node->visited+=1;

    Node * selected_node = cached_initial_node;
    while (!selected_node->is_leaf) {
        selected_node = max_utc_child(selected_node->children);
        selected_node->visited+=1;
    }

    selected_node->is_leaf = false;
    return *selected_node;
}

template<class Entry>
Entry MonteCarloOpenList<Entry>::remove_min() {
    Node &selected_node = select();
    return selected_node.entry;
}

template<class Entry>
MonteCarloOpenList<Entry>::MonteCarloOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
    
    c = 1.414; // sqrt(2) -> TODO: make an option
}

template<class Entry>
bool MonteCarloOpenList<Entry>::empty() const {
    return false;
}

template<class Entry>
void MonteCarloOpenList<Entry>::clear() {

}

template<class Entry>
bool MonteCarloOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool MonteCarloOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void MonteCarloOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

MonteCarloOpenListFactory::MonteCarloOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
MonteCarloOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<MonteCarloOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
MonteCarloOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<MonteCarloOpenList<EdgeOpenListEntry>>(options);
}

class MonteCarloOpenListFeature : public plugins::TypedFeature<OpenListFactory, MonteCarloOpenListFactory> {
public:
    MonteCarloOpenListFeature() : TypedFeature("mcts") {
        document_title("Monte Carlo tree search based open list");
        document_synopsis(
            "Use Monte Carlo selection and backpropogation to guide the search"
            "and balance exploration and heuristic exploitation (greediness).");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<MonteCarloOpenListFeature> _plugin;
}