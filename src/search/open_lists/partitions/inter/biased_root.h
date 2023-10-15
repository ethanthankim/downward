#ifndef PARTITION_POLICIES_BIASED_H
#define PARTITION_POLICIES_BIASED_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_root_partition {
class InterBiasedRootPolicy : public PartitionPolicy {

    std::shared_ptr<Evaluator> evaluator;
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

    std::map< int, std::vector<PartitionNode> > h_buckets;
    utils::HashMap<int, int> node_to_part;
    utils::HashMap<int, std::pair<int, int>> partition_to_id_pair; // the pair of values needed to get partition  from h_buckets
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    double current_sum;

    // int total_gets = 0;
    // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};

private:
    // void verify_heap();
    PartitionNode remove_partition(int partition_key);
    void insert_partition(int new_h, PartitionNode &partition);
    void modify_partition(int last_chosen_bucket, int h_key);

public:
    explicit InterBiasedRootPolicy(const plugins::Options &opts);
    virtual ~InterBiasedRootPolicy() override = default;

    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual void notify_removal(int partition_key, int node_key) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
        evaluator->get_path_dependent_evaluators(evals);
    };
    virtual void clear() {
        h_buckets.clear();
        node_to_part.clear();
        partition_to_id_pair.clear();
        current_sum = 0.0;
    };
    virtual void notify_partition_transition(int parent_part, int child_part) {};
};
}

#endif