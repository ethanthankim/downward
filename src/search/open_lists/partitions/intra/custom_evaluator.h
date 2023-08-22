#ifndef NODE_POLICIES_INTRA_UNIFORM
#define NODE_POLICIES_INTRA_UNIFORM

#include "node_policy.h"

#include "../../../utils/rng.h"
#include "../../../evaluator.h"
#include "../../../utils/rng_options.h"

#include <map>

namespace intra_partition_custom_evaluator {
class IntraCustomEvaluatorPolicy : public NodePolicy {

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::shared_ptr<Evaluator> evaluator;

    double epsilon;
    utils::HashMap<NodeKey, int> h_values;
    NodeKey cached_last_removed;


private:
    bool compare_parent_node_smaller(NodeKey parent, NodeKey child);
    bool compare_parent_node_bigger(NodeKey parent, NodeKey child);
    void adjust_to_top(std::vector<NodeKey> &heap, int pos);
    void adjust_heap_down(std::vector<NodeKey> &heap, int loc);
    void adjust_heap_up(std::vector<NodeKey> &heap, int pos);
    NodeKey random_access_heap_pop(std::vector<NodeKey> &heap, int loc);

    
public:
    explicit IntraCustomEvaluatorPolicy(const plugins::Options &opts);
    virtual ~IntraCustomEvaluatorPolicy() override = default;

    virtual void insert(EvaluationContext &context, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> active_states, std::vector<NodeKey> &partition) override;
    virtual NodeKey remove_next_state_from_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, std::vector<NodeKey> &partition) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif