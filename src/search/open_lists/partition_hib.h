#ifndef OPEN_LISTS_PARTITION_HIB_BASE_H
#define OPEN_LISTS_PARTITION_HIB_BASE_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_hib_base_open_list {

class PartitionHIBBaseOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionHIBBaseOpenListFactory(const plugins::Options &options);
    virtual ~PartitionHIBBaseOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif