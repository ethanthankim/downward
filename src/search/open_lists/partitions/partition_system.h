#ifndef PARTITION_SYSTEM_H
#define PARTITION_SYSTEM_H

#include "../partition_open_list.h"
#include "../../plugins/plugin.h"
#include "../../utils/logging.h"
#include "../../utils/hash.h"


using Key = int;
struct OpenState {
    StateID id;
    StateID parent_id;
    OperatorID creating_op;
    Key partition; 
    int h;
    int g;
    OpenState(StateID &id, StateID &parent_id, OperatorID &creating_op, Key partition, int h, int g) 
        : id(id), parent_id(parent_id), creating_op(creating_op), partition(partition), h(h), g(g) {};
};


class PartitionSystem {
    const std::string description;

protected:
    mutable utils::LogProxy log;

public:
    explicit PartitionSystem(const plugins::Options &opts);
    virtual ~PartitionSystem() = default;

    const std::string &get_description() const;

    bool safe_to_remove_node(Key to_remove);
    bool safe_to_remove_partition(Key to_remove);
    virtual Key insert_state(Key to_insert, const utils::HashMap<Key, OpenState> &open_states) = 0;
    virtual Key select_next_partition(const utils::HashMap<Key, OpenState> &open_states) = 0;
    virtual Key select_next_state_from_partition(Key partition, const utils::HashMap<Key, OpenState> &open_states) = 0;

    // inspiration for logging things
    // void report_value_for_initial_state(const EvaluationResult &result) const;
    // void report_new_minimum_value(const EvaluationResult &result) const;
};

extern void add_partition_options_to_feature(plugins::Feature &feature);


#endif