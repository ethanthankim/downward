#ifndef OPEN_LISTS_PARTITION_BIAS_DEPTH_BIAS_H_H
#define OPEN_LISTS_PARTITION_BIAS_DEPTH_BIAS_H_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace partition_bias_depth_bias_h_open_list {

class PartitionBiasDepthBiasHOpenListFactory : public OpenListFactory {
    plugins::Options options;

public:
    explicit PartitionBiasDepthBiasHOpenListFactory(const plugins::Options &options);
    virtual ~PartitionBiasDepthBiasHOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif