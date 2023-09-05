#ifndef PARTITION_POLICIES_INTER_EG_MINH_H
#define PARTITION_POLICIES_INTER_EG_MINH_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_eg_root_partition {
class InterEpsilonGreedyRootPolicy : public PartitionPolicy {

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;

    struct PartitionNode {
        int partition;
        int size;
        int h;
        PartitionNode(int partition, int size, int h)
            : partition(partition), size(size), h(h) {
        }
        bool operator>(const PartitionNode &other) const {
            return h > other.h;
        }
        bool operator<(const PartitionNode &other) const {
            return h <= other.h;
        }
    };

    std::vector<PartitionNode> partition_heap;

    int last_chosen_partition_index = -1;
    int last_chosen_partition = -1;

private: 
    void adjust_heap_down(int loc); 
    void adjust_heap_up(int loc);   
    void adjust_heap(int loc); 
    int remove_last_chosen_partition();

    bool compare_parent_type_smaller();
    bool compare_parent_type_bigger();

public:
    explicit InterEpsilonGreedyRootPolicy(const plugins::Options &opts);
    virtual ~InterEpsilonGreedyRootPolicy() override = default;

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
        partition_heap.clear();

        last_chosen_partition_index = -1;
        last_chosen_partition = -1;
    };
};
}

#endif