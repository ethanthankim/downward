#ifndef OPEN_LISTS_LWM_INTRA_EXPLORE_BASED_OPEN_LIST_H
#define OPEN_LISTS_LWM_INTRA_EXPLORE_BASED_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_lwm_intra_explore_open_list {
class LWMInvPercOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit LWMInvPercOpenListFactory(const plugins::Options &options);
    virtual ~LWMInvPercOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif