#ifndef OPEN_LISTS_INTER_EP_INTRA_EP_OPEN_LIST_H
#define OPEN_LISTS_INTER_EP_INTRA_EP_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace inter_greedy_intra_epsilon_open_list {
class BTSInterGreedyIntraEpOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSInterGreedyIntraEpOpenListFactory(const plugins::Options &options);
    virtual ~BTSInterGreedyIntraEpOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif