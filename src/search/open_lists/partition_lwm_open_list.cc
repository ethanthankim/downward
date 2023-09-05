#include "partition_lwm_open_list.h"

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

namespace partition_lwm_open_list {
template<class Entry>
class PartitionLWMOpenList : public PartitionOpenList<Entry> {

    utils::HashMap<int, int> lwm_values;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit PartitionLWMOpenList(const plugins::Options &opts);
    virtual ~PartitionLWMOpenList() override = default;

    void notify_initial_state(const State &initial_state) override;


};

template<class Entry>
void PartitionLWMOpenList<Entry>::notify_initial_state(const State &initial_state) {
    PartitionOpenList<Entry>::notify_initial_state(initial_state);
    lwm_values[this->cached_parent_id.get_value()] = numeric_limits<int>::max();
}

template<class Entry>
void PartitionLWMOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int parent_partition = this->partitioned_nodes.at(this->cached_parent_id.get_value()).partition;
    int partition_key;
    bool new_type;
    if ( (new_h < lwm_values[parent_partition])) {
        partition_key = this->cached_next_state_id.get_value();
        lwm_values[partition_key] = new_h;
        new_type = true;
    } else {
        partition_key = this->partitioned_nodes.at(this->cached_parent_id.get_value()).partition;
        new_type = false;
    }

    PartitionOpenList<Entry>::partition_insert(eval_context, new_h, entry, partition_key, new_type);
}

template<class Entry>
PartitionLWMOpenList<Entry>::PartitionLWMOpenList(const plugins::Options &opts)
    : PartitionOpenList<Entry>(
        opts.get<shared_ptr<Evaluator>>("eval"),
        opts.get<shared_ptr<PartitionPolicy>>("partition_policy"),
        opts.get<shared_ptr<NodePolicy>>("node_policy")
    ) {}


PartitionLWMOpenListFactory::PartitionLWMOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionLWMOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionLWMOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionLWMOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionLWMOpenList<EdgeOpenListEntry>>(options);
}

class PartitionLWMOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionLWMOpenListFactory> {
public:
    PartitionLWMOpenListFeature() : TypedFeature("lwm_partition") {
        document_title("Partition Heuristic Improvement Open List");
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

static plugins::FeaturePlugin<PartitionLWMOpenListFeature> _plugin;
}