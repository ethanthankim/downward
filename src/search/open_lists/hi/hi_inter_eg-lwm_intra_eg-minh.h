#ifndef OPEN_LISTS_INTER_EG_LWM_INTRA_EG_MINH_OPEN_LIST_H
#define OPEN_LISTS_INTER_EG_LWM_INTRA_EG_MINH_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace inter_lwm_intra_epsilon_open_list {
class BTSInterLWMIntraEpOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSInterLWMIntraEpOpenListFactory(const plugins::Options &options);
    virtual ~BTSInterLWMIntraEpOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif