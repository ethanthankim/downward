#ifndef PARTITION_POLICIES_INTER_EG_MINH_H
#define PARTITION_POLICIES_INTER_EG_MINH_H

#include "partition_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace inter_eg_minh_partition {
class InterEpsilonGreedyMinHPolicy : public PartitionPolicy {

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;

    int counter = 0;

    struct PartitionNode {
        int partition;
        std::map<int, int> state_hs;
        PartitionNode(int partition, std::map<int, int> state_hs)
            : partition(partition), state_hs(state_hs) {
        }
        bool operator>(const PartitionNode &other) const {
            if (state_hs.empty()) return false;
            if (other.state_hs.empty()) return true;
            return state_hs.begin()->first > other.state_hs.begin()->first;
        }
        bool operator<(const PartitionNode &other) const {
            if (state_hs.empty()) return true;
            if (other.state_hs.empty()) return false;
            return state_hs.begin()->first < other.state_hs.begin()->first;
        }
        bool operator<=(const PartitionNode &other) const {
            if (state_hs.empty()) return true;
            if (other.state_hs.empty()) return false;
            return state_hs.begin()->first <= other.state_hs.begin()->first;
        }
    };

    std::vector<PartitionNode> partition_heap;
    utils::HashMap<int, int> node_hs; 
    // std::map<int, int> node_hs;

private: 
    void adjust_heap_down(int loc); 
    void adjust_heap_up(int loc);   
    void adjust_heap(int loc); 
    int remove_last_chosen_partition();

    bool compare_parent_type_smaller();
    bool compare_parent_type_bigger();

public:
    explicit InterEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~InterEpsilonGreedyMinHPolicy() override = default;

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
        node_hs.clear();
        partition_heap.clear();

    };
    virtual void notify_partition_transition(int parent_part, int child_part) {};
};
}

#endif