#ifndef NODE_POLICIES_INTRA_UNIFORM
#define NODE_POLICIES_INTRA_UNIFORM

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <memory>
#include <map>

namespace intra_partition_uniform {
class IntraUniformPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    utils::HashMap<int, std::vector<int>> node_partitions;

    int last_chosen_partition = -1;
    
public:
    explicit IntraUniformPolicy(const plugins::Options &opts);
    virtual ~IntraUniformPolicy() override = default;

    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) override;
    virtual int get_next_node(int partition_key) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        last_chosen_partition=-1;
        node_partitions.clear();
    };
};
}

#endif