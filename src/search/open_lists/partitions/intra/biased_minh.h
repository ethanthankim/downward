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

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double tau;
    bool ignore_size;
    bool ignore_weights;
    bool relative_h;
    int relative_h_offset; // i think per partition to, but i dont care.

    struct BiasedPartition {
        double current_sum;
        std::map<int, std::vector<int>> h_buckets;
        BiasedPartition(int current_sum, std::map<int, std::vector<int>> h_buckets)
            : current_sum(current_sum), h_buckets(h_buckets) {
        }
    };
    utils::HashMap<int, BiasedPartition> part_id_to_part;

    // int total_gets=0;
    // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};
    
public:
    explicit IntraBiasedPolicy(const plugins::Options &opts);
    virtual ~IntraBiasedPolicy() override = default;

    virtual void notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval
    ) override;
    virtual int get_next_node(int partition_key) override;
    virtual void clear() {
        part_id_to_part.clear();
    };
};
}

#endif