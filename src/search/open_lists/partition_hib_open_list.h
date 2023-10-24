#ifndef OPEN_LISTS_PARTITION_HIB_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_HIB_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_hib_open_list {

class PartitionHIBOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionHIBOpenListFactory(const plugins::Options &options);
    virtual ~PartitionHIBOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif