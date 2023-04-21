#ifndef SEARCH_ENGINES_LAZY_SEARCH_ANYTIME_H
#define SEARCH_ENGINES_LAZY_SEARCH_ANYTIME_H

#include "../open_list.h"
#include "../search_engine.h"

#include <memory>
#include <vector>

class Evaluator;
class PruningMethod;

namespace plugins {
class Feature;
}

namespace lazy_search_anytime {
class LazySearchAnytime : public SearchEngine {
    const bool reopen_closed_nodes;

    std::unique_ptr<StateOpenList> open_list;
    // std::vector<int> weights;
    std::shared_ptr<Evaluator> f_evaluator;
    int best_bound;
    unsigned int num_found_solutions;
    

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:

    explicit LazySearchAnytime(const plugins::Options &opts);
    virtual ~LazySearchAnytime() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
