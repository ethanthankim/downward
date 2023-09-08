#ifndef PARTITION_POLICIES_BIASED_MINH_H
#define PARTITION_POLICIES_BIASED_MINH_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_minh_partition {
class InterBiasedMinHPolicy : public PartitionPolicy {

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    int last_chosen_partition_key = -1;
    int last_chosen_partition_i = -1;
    int last_chosen_key = -1;
    int last_chosen_h = -1;

    struct PartitionNode {
        int partition;
        std::map<int, int>  state_hs;
        PartitionNode(int partition, std::map<int, int> state_hs)
            : partition(partition), state_hs(state_hs) {
        }
    };
    std::map<int, std::vector<PartitionNode>> h_buckets;
    utils::HashMap<int, int> node_hs;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;

private:
    bool maybe_move_last_partition();
    void remove_last_partition();
    void insert_partition(int new_h, PartitionNode &partition);

public:
    explicit InterBiasedMinHPolicy(const plugins::Options &opts);
    virtual ~InterBiasedMinHPolicy() override = default;

    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual void notify_removal(int partition_key, int node_key) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        h_buckets.clear();
    };
};
}

#endif