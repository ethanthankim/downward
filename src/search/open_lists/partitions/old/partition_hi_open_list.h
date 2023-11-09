#ifndef OPEN_LISTS_PARTITION_HI_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_HI_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_hi_open_list {

class PartitionHIOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionHIOpenListFactory(const plugins::Options &options);
    virtual ~PartitionHIOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif