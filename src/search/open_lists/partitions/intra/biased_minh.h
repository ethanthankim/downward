#ifndef NODE_POLICIES_INTRA_BIASED
#define NODE_POLICIES_INTRA_BIASED

#include "node_policy.h"

#include "../../../evaluator.h"
#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <map>
#include <deque>

namespace intra_partition_biased {
class IntraBiasedPolicy : public NodePolicy {

    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset;
    utils::HashMap<int, std::pair<double, std::map<int, std::deque<int>>>> partition_h_buckets;
    int cached_last_removed = -1;
    
public:
    explicit IntraBiasedPolicy(const plugins::Options &opts);
    virtual ~IntraBiasedPolicy() override = default;

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
        partition_h_buckets.clear();
    };
};
}

#endif