#ifndef PARTITION_POLICY_H
#define PARTITION_POLICY_H

#include "../partition_system.h"
#include "../../../plugins/plugin.h"
#include "../../../utils/logging.h"
#include "../../../utils/hash.h"
#include "../../../evaluation_context.h"


class PartitionPolicy {

    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit PartitionPolicy(const plugins::Options &opts);
    virtual ~PartitionPolicy() = default;

    const std::string &get_description() const;

    virtual PartitionKey get_next_partition(
        utils::HashMap<NodeKey, PartitionedState> &active_states, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets) = 0;
        
    virtual void notify_insert(bool new_type, NodeKey inserted, 
        utils::HashMap<NodeKey, PartitionedState> &active_states, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets) = 0;

    virtual void notify_remove(NodeKey removed, 
        utils::HashMap<NodeKey, PartitionedState> &active_states, 
        utils::HashMap<PartitionKey, Partition> &partition_buckets) = 0;
};

extern void add_partition_policy_options_to_feature(plugins::Feature &feature);


#endif