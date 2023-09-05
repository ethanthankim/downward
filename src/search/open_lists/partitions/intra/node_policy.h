#ifndef NODE_POLICY_H
#define NODE_POLICY_H

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

    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) = 0;
    virtual int get_next_node(int partition_key) = 0;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) = 0;
    virtual void clear() = 0;

};

extern void add_node_policy_options_to_feature(plugins::Feature &feature);


#endif