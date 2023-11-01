#ifndef PARTITION_POLICIES_BIASED_MINH_H
#define PARTITION_POLICIES_BIASED_MINH_H

#include "partition_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_minh_partition {
class InterBiasedMinHPolicy : public PartitionPolicy {
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    struct PartitionNode {
        int partition;
        std::map<int, int>  h_counts;
        PartitionNode(int partition, std::map<int, int> h_counts)
            : partition(partition), h_counts(h_counts) {
        }
        inline void inc_h_count(int h) 
        { 
            h_counts[h] += 1;
        }
        inline void dec_h_count(int h) 
        { 
            h_counts[h] -= 1;
            if (h_counts[h] <= 0) {
                h_counts.erase(h);
            }
        }
    };
    std::map<int, std::vector<PartitionNode>> h_buckets;
    utils::HashMap<int, int> node_hs;
    utils::HashMap<int, std::pair<int, int>> partition_to_id_pair; // the pair of values needed to get partition  from h_buckets
    double tau;
    bool ignore_size;
    bool ignore_weights;
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
        int eval) override;
    virtual void notify_removal(int partition_key, int node_key) override;
    virtual void clear() {
        h_buckets.clear();
        node_hs.clear();
        current_sum = 0.0;
    };
    virtual void notify_partition_transition(int parent_part, int parent_node, int child_part, int child_node) {};
};
}

#endif