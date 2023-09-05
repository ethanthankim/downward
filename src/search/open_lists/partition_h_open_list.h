#ifndef OPEN_LISTS_PARTITION_H_OPEN_LIST_H
#define OPEN_LISTS_PARTITION_H_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_h_open_list {

class PartitionHOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionHOpenListFactory(const plugins::Options &options);
    virtual ~PartitionHOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif