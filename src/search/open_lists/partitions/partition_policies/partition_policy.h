#ifndef PARTITION_POLICY_H
#define PARTITION_POLICY_H

#include "../partition_system.h"
#include "../../../plugins/plugin.h"
#include "../../../utils/logging.h"
#include "../../../utils/hash.h"


class PartitionPolicy {

    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit PartitionPolicy(const plugins::Options &opts);
    virtual ~PartitionPolicy() = default;

    const std::string &get_description() const;

    virtual Key remove_min() = 0;
    virtual void notify_insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states) = 0;
    virtual void notify_remove(Key removed, utils::HashMap<Key, PartitionedState> active_states) = 0;


};

extern void add_partition_policy_options_to_feature(plugins::Feature &feature);


#endif