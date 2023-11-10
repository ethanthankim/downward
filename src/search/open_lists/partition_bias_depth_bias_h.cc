#include "partition_bias_depth_bias_h.h"

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
#include <map>
#include <vector>

using namespace std;

namespace partition_bias_depth_bias_h_open_list {
template<class Entry>
class PartitionBiasDepthBiasHOpenList : public OpenList<Entry> {

    shared_ptr<utils::RandomNumberGenerator> rng;
    shared_ptr<Evaluator> evaluator;

    // primary data structures for performing the actual exploration
    struct BiasedPartition {
        int partition_id;
        double current_sum;
        map<int, vector<Entry>> buckets;
        BiasedPartition(int partition_id)
            : partition_id(partition_id), current_sum(0.0), buckets() {
        }

        void insert(int h, Entry entry, double tau, bool ignore_size, bool ignore_weights) {
            
            if (!ignore_size) {
                if (ignore_weights) {
                    current_sum += 1;
                } else {
                    current_sum += std::exp(-1.0 * static_cast<double>(h) / tau);
                }
            }

            buckets[h].push_back(entry);
        }

        Entry remove_min(double tau, bool ignore_size, bool ignore_weights, shared_ptr<utils::RandomNumberGenerator> rng) {
            int h = buckets.begin()->first;
            double r = rng->random();
            double total_sum = current_sum;
            double p_sum = 0.0;
            for (auto it : buckets) {
                double p = 1.0 / total_sum;
                if (!ignore_weights) p *= std::exp(-1.0 * static_cast<double>(it.first) / tau);
                if (!ignore_size) p *= static_cast<double>(it.second.size());
                p_sum += p;
                if (r <= p_sum) {
                    h = it.first;
                    break;
                }
            }
            
            vector<Entry> &bucket = buckets[h];
            assert(!bucket.empty());
            Entry result = utils::swap_and_pop_from_vector(bucket, rng->random(bucket.size()));
            if (bucket.empty()) {
                buckets.erase(h);
                if (ignore_size) {
                    if (ignore_weights)
                        current_sum -= 1;
                    else
                        current_sum -= std::exp(-1.0 * static_cast<double>(h) / tau);
                }
            }
            if (!ignore_size) {
                if (ignore_weights)
                    current_sum -= 1;
                else
                    current_sum -= std::exp(-1.0 * static_cast<double>(h) / tau);
            }
            return result;
        }

        bool empty() {
            return buckets.empty();
        }
    };
    map<int, vector<BiasedPartition>, std::greater<int>> partitions;

    // auxiliary data structures to keep track of node/type locations
    struct PartitionLoc {
        int depth;
        int index;
        PartitionLoc(int depth, int index)
            : order_value(depth), index(index) {
        }
        PartitionLoc() : depth(0), index(0) {}
        inline void set_index(int new_index) {
            index=new_index;
        }
    };
    utils::HashMap<int, PartitionLoc> partition_locs;
    struct StateInfo {
        int id;
        int part_key;
        int h;
        StateInfo(int id, int part_key, int h) : part_key(part_key), h(h) {}
        StateInfo() : id(id), part_key(0), h(0) {}
    };
    PerStateInformation<StateInfo> state_to_info;

    // exploration stuff
    double inter_tau;
    double intra_tau;
    bool intra_ignore_size;
    bool intra_ignore_weights;
    double current_sum;

    //path dependent stuff
    StateInfo curr_expanding_state_info;
    PartitionLoc curr_expand_type_loc;
    bool first_success_in_succ = true;
    int type_counter;
    int next_id;

    int last_removed_from = -1;
    int last_depth = -1;

protected:
    // virtual inline int get_hi_type(int new_h);
    virtual inline BiasedPartition& get_partition_to_insert_into(int new_h);
    virtual inline int add_partition(PartitionLoc& partition_loc, int part_id);
    virtual inline StateInfo add_and_get_node_info(EvaluationContext &eval_context);
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit PartitionBiasDepthBiasHOpenList(const plugins::Options &opts);
    virtual ~PartitionBiasDepthBiasHOpenList() override = default;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;

};

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::notify_initial_state(const State &initial_state) {
    // cached_next_state_id = initial_state.get_id();
    curr_expanding_state_info = StateInfo(-1, numeric_limits<int>::max(), -1);
    curr_expand_type_loc = PartitionLoc(-1, -1);
}

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::notify_state_transition(const State &parent_state,
                                        OperatorID op_id,
                                        const State &state)
{
    StateInfo parent_info = state_to_info[parent_state];
    if (parent_info.id != curr_expanding_state_info.id) {
        curr_expanding_state_info = parent_info;
        first_success_in_succ = true;
    }
}

// template<class Entry>
// inline int PartitionBiasDepthBiasHOpenList<Entry>::get_hi_type(int new_h) {

//     if (new_h < parent_node_info.h)
//         return type_counter++;
//     else 
//         return parent_node_info.type;
// }

template<class Entry>
inline PartitionBiasDepthBiasHOpenList<Entry>::BiasedPartition& 
PartitionBiasDepthBiasHOpenList<Entry>::get_partition_to_insert_into(int part_id) {

    PartitionLoc part_loc;
    if (partition_locs.find(part_id) == partition_locs.end()) {
        part_loc = PartitionLoc(parent_type.order_value+1, 0); // +1 is for depth
        part_loc.index = add_partition(part_loc, part_id);
        partition_locs.emplace(part_id, part_loc);
    } else {
        part_loc = partition_locs.at(part_id);
    }

    return partitions.at(part_loc.order_value)[part_loc.index]; 
}

template<class Entry>
inline int PartitionBiasDepthBiasHOpenList<Entry>::add_partition(PartitionLoc& part_loc, int part_id) {
    if (partitions.find(part_loc.order_value) == partitions.end()) {
        vector<BiasedPartition> fuck_cpp = {BiasedPartition(part_id)};
        partitions.emplace(part_loc.order_value, fuck_cpp);
        current_sum += std::exp(static_cast<double>(part_loc.order_value) / inter_tau);
    } else {
        partitions.at(part_loc.order_value).push_back(BiasedPartition(part_id));
    }
    return partitions.at(part_loc.order_value).size()-1;
}

template<class Entry>
inline PartitionBiasDepthBiasHOpenList<Entry>::StateInfo
PartitionBiasDepthBiasHOpenList<Entry>::add_and_get_node_info(EvaluationContext &eval_context) {
    int h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    StateInfo new_node(get_hi_type(h), min(h, parent_node_info.h));
    node_infos[eval_context.get_state()] = new_node;
    return new_node;
}

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int partition_key;
    PartitionLoc partition_loc;
    if ( (new_h < curr_expanding_state_info.h) ) {
        if (first_success_in_succ) {
            partition_key = type_counter++;
            first_success_in_succ = false;

            int new_depth = curr_expand_type_loc.depth+1;
            partition_loc = PartitionLoc(new_depth, partitions[new_depth].size()); // will auto add vector if empty
            partitions.at(partition_loc.depth).push_back(BiasedPartition(partition_key, 0));
            partition_locs.emplace(partition_key, partition_loc);
        } else {
            partition_key = type_counter-1; // type_counter must have been incremented once.
            partition_loc = partition_locs.at(partition_key);
        } 
    } else {
        partition_loc = curr_expand_type_loc;
        partition_key = curr_expanding_state_info.part_key;
    }

    BiasedPartition &b_part = partitions.at(partition_loc.depth)[partition_loc.index];
    b_part.insert(new_h, entry, intra_tau, intra_ignore_size, intra_ignore_weights);

    state_to_info[eval_context.get_state()] = StateInfo(next_id++, partition_key, new_h);
}

template<class Entry>
Entry PartitionBiasDepthBiasHOpenList<Entry>::remove_min() {

    // if (last_removed_from != -1) {
    //     PartitionLoc last_loc = partition_locs[last_removed_from];
    //     vector<BiasedPartition> &h_partitions = partitions[last_loc.order_value];
    //     BiasedPartition& partition = h_partitions[last_loc.index];
    //     if (partition.empty()) {
    //         partition_locs.erase(partition.partition_id);
    //         utils::swap_and_pop_from_vector(h_partitions, last_loc.index);
    //         if (h_partitions.empty()) {
    //             partitions.erase(last_loc.order_value);
    //             current_sum -= std::exp(static_cast<double>(last_loc.order_value) / inter_tau);
    //         } else if (last_loc.index < h_partitions.size()) {
    //             PartitionLoc& loc = partition_locs.at(h_partitions[last_loc.index].partition_id);
    //             loc.index = last_loc.index;
    //         }
    //     }
    // }

    int selected_depth = partitions.begin()->first;
    if (partitions.size() > 1) {
        double r = rng->random();
        
        double total_sum = current_sum;
        double p_sum = 0.0;
        for (auto it : partitions) {
            double p = 1.0 / total_sum;
            p *= std::exp(static_cast<double>(it.first) / inter_tau);
            p_sum += p;
            if (r <= p_sum) {
                selected_depth = it.first;
                break;
            }
        }
    }

    vector<BiasedPartition> &h_partitions = partitions[selected_depth];
    assert(!h_partitions.empty());

    int part_i = rng->random(h_partitions.size());
    BiasedPartition& partition = h_partitions[part_i];
    Entry result = partition.remove_min(intra_tau, intra_ignore_size, intra_ignore_weights, rng);

    if (partition.empty()) {
        partition_locs.erase(partition.partition_id);
        utils::swap_and_pop_from_vector(h_partitions, part_i);
        if (h_partitions.empty()) {
            partitions.erase(selected_depth);
        }
    }
    current_sum -= std::exp(static_cast<double>(selected_depth) / inter_tau);

    return result;

}


template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::clear() {
    partitions.clear();
    partition_locs.clear();

}

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool PartitionBiasDepthBiasHOpenList<Entry>::empty() const {
    return partitions.empty();
}

template<class Entry>
bool PartitionBiasDepthBiasHOpenList<Entry>::is_dead_end(EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool PartitionBiasDepthBiasHOpenList<Entry>::is_reliable_dead_end(EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
PartitionBiasDepthBiasHOpenList<Entry>::PartitionBiasDepthBiasHOpenList(const plugins::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
      inter_tau(opts.get<double>("inter_tau")),
      intra_tau(opts.get<double>("intra_tau")),
      intra_ignore_size(opts.get<bool>("intra_ignore_size")),
      intra_ignore_weights(opts.get<bool>("intra_ignore_weights")),
      current_sum(0.0),
      type_counter(0),
      next_id(0) {
}

PartitionBiasDepthBiasHOpenListFactory::PartitionBiasDepthBiasHOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
PartitionBiasDepthBiasHOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<PartitionBiasDepthBiasHOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
PartitionBiasDepthBiasHOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<PartitionBiasDepthBiasHOpenList<EdgeOpenListEntry>>(options);
}

class PartitionBiasDepthBiasHOpenListFeature : public plugins::TypedFeature<OpenListFactory, PartitionBiasDepthBiasHOpenListFactory> {
public:
    PartitionBiasDepthBiasHOpenListFeature() : TypedFeature("bias_depth_bias_h") {
        document_title("Partition Heuristic Improvement Open List");
        document_synopsis("A configurable open list that selects nodes by first"
         "choosing a node parition and then choosing a node from within it."
         "The policies for insertion (choosing a partition to insert into or"
         "creating a new one) and selection (partition first, then a node within it)"
         "are  specified policies");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "inter_tau",
            "temperature parameter of inter-exploration", "1.0");
        add_option<double>(
            "intra_tau",
            "temperature parameter of intra-exploration", "1.0");
        add_option<bool>(
            "intra_ignore_size",
            "ignore h bucket sizes", "false");
        add_option<bool>(
            "intra_ignore_weights",
            "ignore linear_weighted weights in intra-exploration", "false");
        utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<PartitionBiasDepthBiasHOpenListFeature> _plugin;
}