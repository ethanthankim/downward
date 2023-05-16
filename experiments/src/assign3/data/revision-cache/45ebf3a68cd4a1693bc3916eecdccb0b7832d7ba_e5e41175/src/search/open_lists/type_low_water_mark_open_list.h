#ifndef OPEN_LISTS_LOW_WATER_MARK_BASED_OPEN_LIST_H
#define OPEN_LISTS_LOW_WATER_MARK_BASED_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


/*
  Low water mark based type system
*/

namespace lwm_based_open_list {
class LWMBasedOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit LWMBasedOpenListFactory(const plugins::Options &options);
    virtual ~LWMBasedOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif