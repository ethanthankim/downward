#ifndef SEARCH_ENGINES_ITERATED_SEARCH_CONTINUED_H
#define SEARCH_ENGINES_ITERATED_SEARCH_CONTINUED_H

#include "../search_engine.h"

#include "../parser/decorated_abstract_syntax_tree.h"

#include <memory>
#include <vector>

namespace iterated_search_continued {
class IteratedSearchContinued : public SearchEngine {
    std::vector<parser::LazyValue> engine_configs;

    bool pass_bound;
    bool f_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<SearchEngine> get_search_engine(int engine_configs_index);
    std::shared_ptr<SearchEngine> create_current_phase();
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    IteratedSearchContinued(const plugins::Options &opts);

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};
}

#endif
