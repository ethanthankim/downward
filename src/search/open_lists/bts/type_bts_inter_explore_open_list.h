#ifndef OPEN_LISTS_BTS_INTRA_EXPLORE_OPEN_LIST_H
#define OPEN_LISTS_BTS_INTRA_EXPLORE_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_bts_inter_explore_open_list {
class BTSInterExploreOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSInterExploreOpenListFactory(const plugins::Options &options);
    virtual ~BTSInterExploreOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif