#ifndef OPEN_LISTS_INTRA_DEPTH_OPEN_LIST_H
#define OPEN_LISTS_INTRA_DEPTH_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_intra_depth_open_list {
class BTSIntraDepthOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSIntraDepthOpenListFactory(const plugins::Options &options);
    virtual ~BTSIntraDepthOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif