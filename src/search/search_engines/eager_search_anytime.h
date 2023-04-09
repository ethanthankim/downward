#ifndef SEARCH_ENGINES_EAGER_SEARCH_ANYTIME_H
#define SEARCH_ENGINES_EAGER_SEARCH_ANYTIME_H

#include "../open_list.h"
#include "../search_engine.h"

#include <memory>
#include <vector>
#include <chrono>

class Evaluator;
class PruningMethod;

namespace plugins {
class Feature;
}

namespace eager_search_anytime {
class EagerSearchAnytime : public SearchEngine {
    const bool reopen_closed_nodes;

    std::unique_ptr<StateOpenList> open_list;
    // std::vector<std::shared_ptr<Evaluator>> evaluators;
    std::shared_ptr<Evaluator> f_evaluator;
    int best_bound;
    unsigned int num_found_solutions;
    

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:

    explicit EagerSearchAnytime(const plugins::Options &opts);
    virtual ~EagerSearchAnytime() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_options_to_feature(plugins::Feature &feature);
}

#endif
