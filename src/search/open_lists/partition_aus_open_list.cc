#include "partition_aus_open_list.h"

#include "partitions/partition_open_list.h"
#include "partitions/inter/partition_policy.h"
#include "partitions/intra/node_policy.h"
#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace partition_open_list;

namespace partition_aus_open_list {
template<class Entry>
class PartitionAusOpenList : public PartitionOpenList<Entry> {

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit PartitionAusOpenList(const plugins::Options &opts);
    virtual ~PartitionAusOpenList() override = default;


};

template<class Entry>
void PartitionAusOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int parent_h = this->partitioned_nodes.at(this->cached_parent_id.get_value()).eval;
    int partition_key;
    bool new_type;
    if ( (new_h < parent_h)) {
        partition_key = this->cached_next_state_id.get_value();
        new_type = true;
    } else {
        partition_key = 0;
        new_type = false;
    }

    PartitionOpenList<Entry>::partition_insert(eval_context, new_h, entry, partition_key, new_type);
    
}

template<class Entry>
PartitionAusOpenList<Entry>::PartitionAusOpenList(const plugins::Options &opts)
    : PartitionOpenList<Entry>(
        opts.get<shared_ptr<Evaluator>>("eval"),
        opts.get<shared_ptr<PartitionPolicy>>("partition_policy"),
        opts.get<shared_ptr<NodePolicy>>("node_policy")
    ) {}


PartitionAusOpenListFactory::PartitionAusOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionAusOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionAusOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionAusOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionAusOpenList<EdgeOpenListEntry>>(options);
}

class PartitionAusOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionAusOpenListFactory> {
public:
    PartitionAusOpenListFeature() : TypedFeature("h_partition") {
        document_title("Partition Low Water-mark Open List");
        document_synopsis("A configurable open list that selects nodes by first"
         "choosing a node parition and then choosing a node from within it."
         "The policies for insertion (choosing a partition to insert into or"
         "creating a new one) and selection (partition first, then a node within it)"
         "are  specified policies");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<shared_ptr<PartitionPolicy>>("partition_policy", "partition selection policy");
        add_option<shared_ptr<NodePolicy>>("node_policy", "node selection policy");
    }
};

static plugins::FeaturePlugin<PartitionAusOpenListFeature> _plugin;
}