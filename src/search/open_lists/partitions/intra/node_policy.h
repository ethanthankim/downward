#ifndef NODE_POLICY_H
#define NODE_POLICY_H

#include "../partition_system.h"
#include "../../../plugins/plugin.h"
#include "../../../utils/logging.h"
#include "../../../utils/hash.h"
#include "../../../evaluation_context.h"


class NodePolicy {

    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit NodePolicy(const plugins::Options &opts);
    virtual ~NodePolicy() = default;

    const std::string &get_description() const;

    virtual void insert(Key inserted, utils::HashMap<Key, PartitionedState> active_states, std::vector<Key> &partition) = 0;
    virtual Key remove_next_state_from_partition(utils::HashMap<Key, PartitionedState> &active_states, std::vector<Key> &partition) = 0;


};

extern void add_node_policy_options_to_feature(plugins::Feature &feature);


#endif