#ifndef OPEN_LISTS_TYPE_BASED_PATH_OPEN_LIST_H
#define OPEN_LISTS_TYPE_BASED_PATH_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"


namespace type_based_path_open_list {
class TypeBasedPathOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit TypeBasedPathOpenListFactory(const plugins::Options &options);
    virtual ~TypeBasedPathOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
