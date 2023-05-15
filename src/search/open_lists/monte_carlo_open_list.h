#ifndef OPEN_LISTS_MONTE_CARLO_OPEN_LIST_H
#define OPEN_LISTS_MONTE_CARLO_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


/*
  Monte carlo based open list
*/

namespace monte_carlo_open_list {
class MonteCarloOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit MonteCarloOpenListFactory(const plugins::Options &options);
    virtual ~MonteCarloOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif