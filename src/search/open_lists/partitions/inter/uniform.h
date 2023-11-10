#ifndef PARTITION_POLICIES_UNIFORM_H
#define PARTITION_POLICIES_UNIFORM_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <memory>
#include <map>

namespace inter_uniform_partition {
class InterUniformPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    
    std::vector<std::pair<int, int>> partition_keys_and_sizes;
    utils::HashMap<int, int> key_to_partition_index;

public:
    explicit InterUniformPolicy(const plugins::Options &opts);
    virtual ~InterUniformPolicy() override = default;

    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) override;
    virtual void notify_removal(int partition_key, int node_key) override;
    virtual void clear() {
        partition_keys_and_sizes.clear();
        key_to_partition_index.clear();
    };
    virtual void notify_partition_transition(int parent_part, int child_part) {};
};
}

#endif