#ifndef PARTITION_POLICY_H
#define PARTITION_POLICY_H


#include "../../../plugins/plugin.h"
#include "../../../utils/logging.h"
#include "../../../utils/hash.h"
#include "../../../evaluation_context.h"

#include <set>

class PartitionPolicy {

    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit PartitionPolicy(const plugins::Options &opts);
    virtual ~PartitionPolicy() = default;

    const std::string &get_description() const;

    virtual int get_next_partition() = 0;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context
    ) = 0;
    virtual void notify_removal(int partition_key, int node_key) = 0;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) = 0;
    virtual void clear() = 0;
};

extern void add_partition_policy_options_to_feature(plugins::Feature &feature);


#endif