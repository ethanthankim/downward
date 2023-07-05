#ifndef OPEN_LISTS_LWM_INTRA_DEPTH_OPEN_LIST_H
#define OPEN_LISTS_LWM_INTRA_DEPTH_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_lwm_intra_depth_open_list {
class LWMIntraDepthOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit LWMIntraDepthOpenListFactory(const plugins::Options &options);
    virtual ~LWMIntraDepthOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif