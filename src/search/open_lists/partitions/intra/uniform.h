#ifndef NODE_POLICIES_INTRA_UNIFORM
#define NODE_POLICIES_INTRA_UNIFORM

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <memory>
#include <map>

namespace intra_uniform_partition {
class IntraUniformPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    utils::HashMap<int, std::vector<int>> node_partitions;
    
public:
    explicit IntraUniformPolicy(const plugins::Options &opts);
    virtual ~IntraUniformPolicy() override = default;

    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) override;
    virtual int get_next_node(int partition_key) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) {};
    virtual void clear() {
        node_partitions.clear();
    };
};
}

#endif