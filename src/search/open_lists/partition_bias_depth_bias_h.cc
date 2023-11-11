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
            
            if (ignore_size) {
                if (buckets.find(h) == buckets.end()) {
                    if (ignore_weights)
                        current_sum += 1;
                    else
                        current_sum += std::exp(-1.0 * static_cast<double>(h) / tau);
                }
            } else {
                if (ignore_weights)
                    current_sum += 1;
                else
                    current_sum += std::exp(-1.0 * static_cast<double>(h) / tau);

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
            : depth(depth), index(index) {
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
        int part_depth;
        StateInfo(int id, int part_key, int h, int part_depth) : id(id), part_key(part_key), h(h), part_depth(part_depth) {}
        StateInfo() : id(-1), part_key(0), h(0) {}
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


    // // std::vector<int> counts = {0,0,0,0,0,0,0,0,0,0};
    // int first_count = 0;
    // int second_count = 0;
    // int third_count = 0;

protected:
    // void verify_heap();
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

// template<class Entry>
// void PartitionBiasDepthBiasHOpenList<Entry>::verify_heap() {

//     bool valid = true;
//     for (auto &b : partitions) {
//         int depth = b.first;
//         int i = 0;
//         for (auto &p: b.second) {
//             PartitionLoc &p_loc = partition_locs.at(p.partition_id);
//             if (p_loc.depth != depth) {
//                 valid = false;
//             }
//             if (p_loc.index != i) {
//                 valid = false;
//             }
//             if (p.empty()) {
//                 valid = false;
//             }
//             if (p.current_sum < 0) {
//                 valid = false;
//             }

//             if (!valid) {
//                 cout << "Ahhh the heap isn't valid..." << endl; 
//                 exit(1);
//             }
//             i+=1;
//         }
//     }

// }

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::notify_initial_state(const State &initial_state) {
    // cached_next_state_id = initial_state.get_id();
    curr_expanding_state_info = StateInfo(-1, -1, numeric_limits<int>::max(), -1);
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

template<class Entry>
void PartitionBiasDepthBiasHOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    
    int new_h = eval_context.get_evaluator_value_or_infinity(this->evaluator.get());
    int new_depth;
    int partition_key;
    if ( (new_h < curr_expanding_state_info.h) ) {
        if (first_success_in_succ) {
            partition_key = type_counter++;
            first_success_in_succ = false;
            new_depth = curr_expanding_state_info.part_depth+1;
            current_sum += std::exp(static_cast<double>(new_depth) / inter_tau);
        } else {
            partition_key = type_counter-1; // type_counter must have been incremented once.
            new_depth = curr_expanding_state_info.part_depth+1;
        } 
    } else {
        partition_key = curr_expanding_state_info.part_key;
        new_depth = curr_expanding_state_info.part_depth;
    }

    if (partition_locs.find(partition_key) == partition_locs.end()) { // add empty and removed partition back (can happen with path dependent systems)
        partitions[new_depth].push_back(BiasedPartition(partition_key));
        partition_locs.emplace(partition_key, PartitionLoc(new_depth, partitions[new_depth].size()-1));
    }
    PartitionLoc partition_loc = partition_locs[partition_key];

    BiasedPartition &b_part = partitions.at(partition_loc.depth)[partition_loc.index];
    b_part.insert(new_h, entry, intra_tau, intra_ignore_size, intra_ignore_weights);

    state_to_info[eval_context.get_state()] = StateInfo(next_id++, partition_key, new_h, new_depth);
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

    // if (next_id % 100 == 0) {
    //     cout << "verify" << endl;
    //     verify_heap();
    // }

    int selected_depth = partitions.begin()->first;
    if (partitions.size() > 1) {
        double r = rng->random();
        
        double total_sum = current_sum;
        double p_sum = 0.0;
        for (auto it : partitions) {
            double p = 1.0 / total_sum;
            p *= std::exp(static_cast<double>(it.first) / inter_tau);
            p *= static_cast<double>(it.second.size());
            // cout << p <<endl;
            p_sum += p;
            if (r <= p_sum) {
                selected_depth = it.first;
                break;
            }
        }
    }

    // int i=0;
    // for (auto it : partitions) {
    //     if (selected_depth == it.first) {
    //         if (i==0) first_count +=1;
    //         if (i==1) first_count +=1;
    //         if (i==2) first_count +=1;
    //     } 
    //     i+=1;
    // }

    // if (next_id % 200 == 0) {
    //     cout << first_count << " : " << second_count << " : " << third_count << endl;
    // }


    vector<BiasedPartition> &h_partitions = partitions[selected_depth];
    assert(!h_partitions.empty());

    int part_i = rng->random(h_partitions.size());
    BiasedPartition& partition = h_partitions[part_i];
    Entry result = partition.remove_min(intra_tau, intra_ignore_size, intra_ignore_weights, rng);

    if (partition.empty()) {
        int temp = h_partitions.back().partition_id;
        partition_locs[temp].index = part_i;
        partition_locs.erase(partition.partition_id);
        utils::swap_and_pop_from_vector(h_partitions, part_i);
        if (h_partitions.empty()) {
            partitions.erase(selected_depth);
        }
        current_sum -= std::exp(static_cast<double>(selected_depth) / inter_tau);
    }

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