#include "partition_hib_open_list.h"

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

#include <chrono>

#include <fstream>

using namespace std;
using namespace partition_open_list;

namespace partition_hib_open_list {
template<class Entry>
class PartitionHIBOpenList : public PartitionOpenList<Entry> {

    int curr_expanding = -1;
    int last_removed = -1;
    int curr_expanding_h;
    int curr_expanding_part_key = -1;

    bool first_success_in_succ = true;
    bool first_gen = true;

    PerStateInformation<int> state_to_id;
    int type_counter;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    Entry remove_min() override;
    explicit PartitionHIBOpenList(const plugins::Options &opts);
    virtual ~PartitionHIBOpenList() override = default;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;

    virtual bool empty() const override;


};

template<class Entry>
void PartitionHIBOpenList<Entry>::notify_initial_state(const State &initial_state) {
    first_gen = true;
    curr_expanding_h = numeric_limits<int>::max();
}

template<class Entry>
void PartitionHIBOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    int parent_id = state_to_id[parent_state];
    if (parent_id != curr_expanding) {
        curr_expanding = parent_id;
        first_success_in_succ = true;
        first_gen = true;
    } else {
        first_gen = false;
    }

    curr_expanding_part_key = this->partitioned_nodes.at(curr_expanding).first.partition;
    curr_expanding_h = this->partitioned_nodes.at(curr_expanding).first.eval;
}

template<class Entry>
void PartitionHIBOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {

    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    bool is_new_part = false;
    int partition_key;
    if ( (new_h < curr_expanding_h) ) {
        if (first_success_in_succ) {

            // if (type_counter % 500 == 0) {
            //     cout << "Type count: " << type_counter << endl;
            // }

            partition_key = type_counter++;
            is_new_part = true;
            first_success_in_succ = false;
        } else {
            partition_key = type_counter-1; // type_counter must have been incremented once.
        }
    } else {
        partition_key = curr_expanding_part_key;
    }
    this->partition_selector->notify_partition_transition(
            curr_expanding_part_key, 
            partition_key, 
            first_gen);
    int id = PartitionOpenList<Entry>::partition_insert( 
        new_h, 
        entry, 
        partition_key, 
        is_new_part);
    state_to_id[eval_context.get_state()] = id;

    // ofstream myfile;
    // myfile.open ("/home/dawson/Documents/Masters/Thesis/search/thesis/benches/media/_ht.txt", std::ios_base::app);
    // myfile << "NODE:" << curr_expanding << ":" << id << ":" << partition_key <<endl;
    // myfile.close();

}

template<class Entry>
Entry PartitionHIBOpenList<Entry>::remove_min() {

    if (last_removed != -1) {
        this->partition_selector->notify_removal(this->partitioned_nodes.at(last_removed).first.partition, last_removed);
        this->partitioned_nodes.erase(last_removed);
    }

    int chosen_partition = this->partition_selector->get_next_partition();
    int chosen_node = this->node_selector->get_next_node(chosen_partition);

    Entry result = this->partitioned_nodes.at(chosen_node).second;
    last_removed = chosen_node;

    return result;

}

template<class Entry>
bool PartitionHIBOpenList<Entry>::empty() const {
    return this->partitioned_nodes.empty();
}


template<class Entry>
PartitionHIBOpenList<Entry>::PartitionHIBOpenList(const plugins::Options &opts)
    : PartitionOpenList<Entry>(
        opts.get<shared_ptr<Evaluator>>("eval"),
        opts.get<shared_ptr<PartitionPolicy>>("partition_policy"),
        opts.get<shared_ptr<NodePolicy>>("node_policy")
    ),
    type_counter(0) {}


PartitionHIBOpenListFactory::PartitionHIBOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionHIBOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionHIBOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionHIBOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionHIBOpenList<EdgeOpenListEntry>>(options);
}

class PartitionHIBOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionHIBOpenListFactory> {
public:
    PartitionHIBOpenListFeature() : TypedFeature("hib_partition") {
        document_title("Partition Heuristic Improvement Bench Open List");
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

static plugins::FeaturePlugin<PartitionHIBOpenListFeature> _plugin;
}