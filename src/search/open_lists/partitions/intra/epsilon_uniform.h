#ifndef NODE_POLICIES_EPSILON_UNIFORM
#define NODE_POLICIES_EPSILON_UNIFORM

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../utils/rng_options.h"

#include <memory>
#include <map>

namespace intra_epsilon_uniform {
class EpsilonUniformPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;

    struct Node {
        int id;
        int eval;
        Node(int id, int eval) : id(id), eval(eval) {}
    };
    utils::HashMap<int, std::vector<Node>> node_partitions;
    double epsilon;
    
public:
    explicit EpsilonUniformPolicy(const plugins::Options &opts);
    virtual ~EpsilonUniformPolicy() override = default;

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