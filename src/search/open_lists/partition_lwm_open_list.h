#ifndef OPEN_LISTS_PARTITION_LWM_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_LWM_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_lwm_open_list {

class PartitionLWMOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionLWMOpenListFactory(const plugins::Options &options);
    virtual ~PartitionLWMOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif