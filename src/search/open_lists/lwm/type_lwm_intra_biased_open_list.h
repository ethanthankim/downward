#ifndef OPEN_LISTS_LWM_INTRA_BIASED_OPEN_LIST_H
#define OPEN_LISTS_LWM_INTRA_BIASED_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_lwm_intra_biased_open_list {
class LWMIntraBiasedOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit LWMIntraBiasedOpenListFactory(const plugins::Options &options);
    virtual ~LWMIntraBiasedOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif