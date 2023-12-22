#ifndef PARTITION_POLICIES_BIASED_LEVEL_H
#define PARTITION_POLICIES_BIASED_LEVEL_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_biased_level_partition {
class InterBiasedLevelPolicy : public PartitionPolicy {

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
    std::map< int, std::vector<PartitionNode>, std::less<int> > h_buckets;
    utils::HashMap<int, std::pair<int, int>> partition_to_id_pair; // the pair of values needed to get partition  from h_buckets


    int curr_parent;
    std::vector<std::pair<int, PartitionNode>> new_partition_with_level_cache;
    // int total_gets=0;
    // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};

    double tau;
    bool ignore_size;
    double current_sum;

private:
    inline int partition_index_in_cache(int partition_key) {
        int i=0;
        for (auto p : new_partition_with_level_cache) {
            if (p.second.partition == partition_key) return i;
            i+=1;
        }
        return i;
    }


public:
    explicit InterBiasedLevelPolicy(const plugins::Options &opts);
    virtual ~InterBiasedLevelPolicy() override = default;

    PartitionNode remove_partition(int partition_key);
    void insert_partition(int new_h, PartitionNode &partition);
    void modify_partition(int last_chosen_bucket, int h_key);

    // void verify_heap();
    virtual int get_next_partition() override;
    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) override;
    virtual void notify_removal(int partition_key, int node_key);
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        h_buckets.clear();
        // node_to_part.clear();
        partition_to_id_pair.clear();
    };
    virtual void notify_partition_transition(int parent_part, int child_part) override;
};
}

#endif