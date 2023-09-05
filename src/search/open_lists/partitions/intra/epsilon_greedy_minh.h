#ifndef NODE_POLICIES_INTRA_EG_MINH_H
#define NODE_POLICIES_INTRA_EG_MINH_H

#include "node_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"


namespace intra_partition_eg_minh {
class IntraEpsilonGreedyMinHPolicy : public NodePolicy {

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double epsilon;
    
    struct HeapNode {
        int id;
        int h;
        HeapNode(int id, int h)
            : id(id), h(h) {
        }
        bool operator>(const HeapNode &other) const {
            return std::make_pair(h, id) > std::make_pair(other.h, other.id);
        }
    };
    utils::HashMap<int, std::vector<HeapNode>> partition_heaps;
    int cached_last_removed = -1;

public:
    explicit IntraEpsilonGreedyMinHPolicy(const plugins::Options &opts);
    virtual ~IntraEpsilonGreedyMinHPolicy() override = default;

    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context
    ) override;
    virtual int get_next_node(int partition_key) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
        evaluator->get_path_dependent_evaluators(evals);
    };
    virtual void clear() {
        cached_last_removed = -1;
        partition_heaps.clear();
    };
};

}

#endif