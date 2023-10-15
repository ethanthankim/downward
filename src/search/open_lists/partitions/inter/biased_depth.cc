#include "biased_depth.h"

using namespace std;

namespace inter_biased_depth_partition {

InterBiasedDepthPolicy::InterBiasedDepthPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    tau(opts.get<double>("tau")),
    ignore_size(opts.get<bool>("ignore_size")),
    ignore_weights(opts.get<bool>("ignore_weights")),
    relative_h(opts.get<bool>("relative_h")),
    relative_h_offset(opts.get<int>("relative_h_offset")),
    current_sum(0.0) {}

// void InterBiasedDepthPolicy::verify_heap() {

//     bool valid = true;
//     for (auto &b : h_buckets) {
//         int type_h = b.first;
//         int i = 0;
//         for (auto &p: b.second) {
//             auto &p_ids = partition_to_id_pair.at(p.partition);
//             if (p_ids.first != type_h) {
//                 valid = false;
//             }
//             if (p_ids.second != i) {
//                 valid = false;
//             }
//             if (p.size == 0) {
//                 valid = false;
//             }


//             if (!valid) {
//                 cout << "Ahhh the heap isn't valid... type_h:" <<  type_h 
//                     << ", p_ids.first:" << p_ids.first
//                     << ", p_ids.second:" << p_ids.second
//                     << ", i:" << i 
//                     << ", size:" << p.size << endl; 
//                 exit(1);
//             }
//             i+=1;
//         }
//     }

// }

void InterBiasedDepthPolicy::notify_partition_transition(int parent_part, int child_part) {
    if (parent_part == -1) return;
    cached_parent_part = parent_part;
    cached_parent_depth = partition_to_id_pair.at(cached_parent_part).first;
}

void InterBiasedDepthPolicy::insert_partition(int new_h, PartitionNode &partition) {
    bool h_absent = h_buckets.find(new_h) == h_buckets.end();
    if (ignore_size) {
        if (h_absent) {
            if (ignore_weights)
                current_sum += 1;
            else if (!relative_h)
                current_sum += std::exp(static_cast<double>(new_h) / tau);
        }
    } else {
        if (ignore_weights)
            current_sum += 1;
        else if (!relative_h)
            current_sum += std::exp(static_cast<double>(new_h) / tau);
    }
    h_buckets[new_h].push_back(partition);
    partition_to_id_pair.emplace(partition.partition, make_pair(new_h, h_buckets[new_h].size()-1)) ;
}

int InterBiasedDepthPolicy::get_next_partition() {
    // int count_i = 0;
    // total_gets+=1;
    // if (total_gets % 100 == 0) {
    //     verify_heap();
    // }

    int selected_h = h_buckets.begin()->first;
    if (h_buckets.size() > 1) {
        double r = rng->random();
        if (relative_h) {
            double total_sum = 0;
            int i = relative_h_offset;
            for (auto it : h_buckets) {
                double s = std::exp(static_cast<double>(i) / tau); //remove -1.0 *  
                if (!ignore_size) s *= static_cast<double>(it.second.size());
                total_sum += s;
                ++i;
            }
            double p_sum = 0.0;
            i = relative_h_offset;
            for (auto it : h_buckets) {
                double p = std::exp(static_cast<double>(i) / tau) / total_sum; //remove -1.0 * 
                if (!ignore_size) p *= static_cast<double>(it.second.size());
                p_sum += p;
                ++i;
                if (r <= p_sum) {
                    selected_h = it.first;
                    break;
                }
            }
        } else {
            double total_sum = current_sum;
            double p_sum = 0.0;
            for (auto it : h_buckets) {
                double p = 1.0 / total_sum;
                if (!ignore_weights) p *= std::exp(static_cast<double>(it.first) / tau); //remove -1.0 *
                if (!ignore_size) p *= static_cast<double>(it.second.size());
                p_sum += p;
                if (r <= p_sum) {
                    selected_h = it.first;
                    break;
                }
                // count_i+=1; 
            }
        }
    }

    // if (count_i < counts.size()) {
    //     counts[count_i] += 1;
    // }

    // if (total_gets % 100 == 0) {
    //     cout << counts << endl;
    //     cout << h_buckets.begin()->first << " --> " << (++h_buckets.begin())->first << endl;
    // }

    vector<PartitionNode> &partitions = h_buckets[selected_h];
    assert(!partitions.empty());

    return  rng->choose(partitions)->partition;    
}

void InterBiasedDepthPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) 
{
    node_to_part.emplace(node_key, partition_key);
    if (new_partition) {
        auto new_partition = PartitionNode(partition_key, 1);
        insert_partition(cached_parent_depth + 1, new_partition);
    } else {
        auto partition_ids = partition_to_id_pair.at(partition_key); 
        h_buckets.at(partition_ids.first)[partition_ids.second].inc_size();
    }
}

InterBiasedDepthPolicy::PartitionNode InterBiasedDepthPolicy::remove_partition(int partition_key) {
    auto partition_ids = partition_to_id_pair.at(partition_key); 
    auto partition = utils::swap_and_pop_from_vector(h_buckets.at(partition_ids.first), partition_ids.second);

    auto& p_from_back = h_buckets.at(partition_ids.first)[partition_ids.second];
    partition_to_id_pair.at(p_from_back.partition).second = partition_ids.second;

    if (h_buckets.at(partition_ids.first).empty()) {
        h_buckets.erase(partition_ids.first);
        if (ignore_size) {
            if (ignore_weights)
                current_sum -= 1;
            else if (!relative_h)
                current_sum -= std::exp(static_cast<double>(partition_ids.first) / tau);
        }
    }
    if (!ignore_size) {
        if (ignore_weights)
            current_sum -= 1;
        else if (!relative_h)
            current_sum -= std::exp(static_cast<double>(partition_ids.first) / tau);
    }

    partition_to_id_pair.erase(partition_key);
    return partition;
}

void InterBiasedDepthPolicy::notify_removal(int partition_key, int node_key) {
    auto& partition_ids = partition_to_id_pair.at(partition_key);
    auto &last_partition_dfn = h_buckets.at(partition_ids.first)[partition_ids.second];
    last_partition_dfn.dec_size();
    if (last_partition_dfn.size == 0) {
        remove_partition(partition_key);
    }
    node_to_part.erase(node_key);
}


class InterBiasedDepthPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterBiasedDepthPolicy> {
public:
    InterBiasedDepthPolicyFeature() : TypedFeature("inter_biased_depth") {
        document_subcategory("partition_policies");
        document_title("Biased depth partition selection");
        document_synopsis(
            "Choose the next partition biased in favour of high depth partitions.");
        add_option<bool>(
            "pref_only",
            "insert only nodes generated by preferred operators", "false");
        add_option<double>(
            "tau",
            "temperature parameter of softmin", "1.0");
        add_option<bool>(
            "ignore_size",
            "ignore bucket sizes", "false");
        add_option<bool>(
            "ignore_weights",
            "ignore weights of buckets", "false");
        add_option<bool>(
            "relative_h",
            "use relative positions of h-values", "false");
        add_option<int>(
            "relative_h_offset",
            "starting value of relative h-values", "0");
        utils::add_rng_options(*this);
        add_partition_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterBiasedDepthPolicyFeature> _plugin;
}