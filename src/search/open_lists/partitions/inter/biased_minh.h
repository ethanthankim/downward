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

    struct PartitionNode {
        int partition;
        std::map<int, int>  h_counts;
        PartitionNode(int partition, std::map<int, int> h_counts)
            : partition(partition), h_counts(h_counts) {
        }
        void inc_h_count(int h) 
        { 
            h_counts[h] += 1;
        }
        void dec_h_count(int h) 
        { 
            h_counts[h] -= 1;
            if (h_counts[h] <= 0) {
                h_counts.erase(h);
            }
        }
    };
    std::map<int, std::vector<PartitionNode>> h_buckets;
    utils::HashMap<int, int> node_hs;
    utils::HashMap<int, std::pair<int, int>> partition_to_id_pair;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;

    // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};
    // int total_gets = 0;

private:
    // void verify_heap();

    bool maybe_move_partition(int partition_key);
    PartitionNode remove_partition(int partition_key);
    void insert_partition(int new_h, PartitionNode &partition);
    void modify_partition(int last_chosen_bucket, int h_key);

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
        node_hs.clear();
    };
};
}

#endif