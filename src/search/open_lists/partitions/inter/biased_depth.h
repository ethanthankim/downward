#ifndef PARTITION_POLICIES_BIASED_DEPTH_H
#define PARTITION_POLICIES_BIASED_DEPTH_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_depth_partition {
class InterBiasedDepthPolicy : public PartitionPolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;

    struct PartitionNode {
        int partition;
        int  size;
        PartitionNode(int partition, int size)
            : partition(partition), size(size) {
        }
        inline void inc_size() 
        { 
            size += 1;
        }
        inline void dec_size() 
        { 
            size -= 1;
        }
    };
    std::map< int, std::vector<PartitionNode>, std::greater<int> > h_buckets;
    utils::HashMap<int, int> node_to_part;
    utils::HashMap<int, std::pair<int, int>> partition_to_id_pair; // the pair of values needed to get partition  from h_buckets

    int cached_parent_part = -1;
    int cached_parent_depth = -1;

    int total_gets=0;
    // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};

    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;

public:
    explicit InterBiasedDepthPolicy(const plugins::Options &opts);
    virtual ~InterBiasedDepthPolicy() override = default;

    PartitionNode remove_partition(int partition_key);
    void insert_partition(int new_h, PartitionNode &partition);
    void modify_partition(int last_chosen_bucket, int h_key);

    // void verify_heap();
    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual void notify_removal(int partition_key, int node_key);
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        h_buckets.clear();
        node_to_part.clear();
        partition_to_id_pair.clear();
    };
    virtual void notify_partition_transition(int parent_part, int child_part) override;
};
}

#endif