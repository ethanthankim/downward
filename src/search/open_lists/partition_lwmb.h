#ifndef OPEN_LISTS_PARTITION_LWMB_BASE_H
#define OPEN_LISTS_PARTITION_LWMB_BASE_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_lwmb_base_open_list {

class PartitionLWMBBaseOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionLWMBBaseOpenListFactory(const plugins::Options &options);
    virtual ~PartitionLWMBBaseOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif