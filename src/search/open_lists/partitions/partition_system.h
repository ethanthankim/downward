#ifndef PARTITION_SYSTEM_H
#define PARTITION_SYSTEM_H

#include "../../state_id.h"
#include "../../plugins/plugin.h"
#include "../../utils/logging.h"
#include "../../utils/hash.h"
#include "../../operator_id.h"
#include "../../task_proxy.h"


using Key = int;
struct PartitionedState {
    StateID id = StateID::no_state;
    Key partition;
    int h;
    int g; 
    PartitionedState() {};
    PartitionedState(StateID &id, Key partition, int h, int g) 
        : id(id), partition(partition), h(h), g(g) {};
};


class PartitionSystem {
    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit PartitionSystem(const plugins::Options &opts);
    virtual ~PartitionSystem() = default;

    const std::string &get_description() const;

    virtual Key choose_state_partition(utils::HashMap<Key, PartitionedState> active_states);
    virtual void notify_initial_state(const State &initial_state) {};
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) {};

    // inspiration for logging things
    // void report_value_for_initial_state(const EvaluationResult &result) const;
    // void report_new_minimum_value(const EvaluationResult &result) const;
};

extern void add_partition_options_to_feature(plugins::Feature &feature);


#endif