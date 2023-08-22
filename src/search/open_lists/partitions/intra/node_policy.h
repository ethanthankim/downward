#ifndef NODE_POLICY_H
#define NODE_POLICY_H

#include "../partition_system.h"
#include "../../../plugins/plugin.h"
#include "../../../utils/logging.h"
#include "../../../utils/hash.h"
#include "../../../evaluation_context.h"

#include <set>

class NodePolicy {

    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit NodePolicy(const plugins::Options &opts);
    virtual ~NodePolicy() = default;

    const std::string &get_description() const;

    virtual void insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) = 0;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) = 0;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};

};

extern void add_node_policy_options_to_feature(plugins::Feature &feature);


#endif