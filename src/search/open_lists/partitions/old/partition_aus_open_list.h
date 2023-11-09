#ifndef OPEN_LISTS_PARTITION_AUS_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_AUS_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_aus_open_list {

class PartitionAusOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionAusOpenListFactory(const plugins::Options &options);
    virtual ~PartitionAusOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif