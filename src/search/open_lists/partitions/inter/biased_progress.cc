#include "biased_progress.h"

using namespace std;

namespace inter_biased_progress_partition {

InterBiasedProgressPolicy::InterBiasedProgressPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    tau(opts.get<double>("tau")/2),
    tau_limit(opts.get<double>("tau")),
    ignore_size(opts.get<bool>("ignore_size")),
    current_sum(0.0),
    node_count(2.0),
    success_count(1.0) {}

// void InterBiasedProgressPolicy::verify_heap() {

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

void InterBiasedProgressPolicy::notify_partition_transition(int parent_part, int child_part) {
    if (parent_part == -1) return;
    cached_parent_part = parent_part;
    cached_parent_depth = partition_to_id_pair.at(cached_parent_part).first;
}

void InterBiasedProgressPolicy::insert_partition(int new_h, PartitionNode &partition) {
    bool h_absent = h_buckets.find(new_h) == h_buckets.end();
    if (ignore_size) {
        if (h_absent) {
            current_sum += std::exp(static_cast<double>(new_h) / tau);
        }
    } else {
        current_sum += std::exp(static_cast<double>(new_h) / tau);
    }
    h_buckets[new_h].push_back(partition);
    partition_to_id_pair.emplace(partition.partition, make_pair(new_h, h_buckets[new_h].size()-1)) ;
}

int InterBiasedProgressPolicy::get_next_partition() {
    // int count_i = 0;
    // total_gets+=1;
    // if (total_gets % 100 == 0) {
    //     verify_heap();
    // }

    int selected_h = h_buckets.begin()->first;
    if (h_buckets.size() > 1) {
        double r = rng->random();
        
        double total_sum = current_sum;
        double p_sum = 0.0;
        for (auto it : h_buckets) {
            double p = 1.0 / total_sum;
            p *= std::exp(static_cast<double>(it.first) / tau); //remove -1.0 *
            if (!ignore_size) p *= static_cast<double>(it.second.size());
            p_sum += p;
            if (r <= p_sum) {
                selected_h = it.first;
                break;
            }
            // count_i+=1; 
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

void InterBiasedProgressPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) 
{
    if (new_partition || partition_to_id_pair.count(partition_key)==0) {
        auto new_partition = PartitionNode(partition_key, 1);
        node_to_prog.emplace(node_key, cached_parent_depth + 1);
        insert_partition(cached_parent_depth + 1, new_partition);
    } else {
        auto partition_ids = partition_to_id_pair.at(partition_key); 
        node_to_prog.emplace(node_key, max(cached_parent_depth - 1, 0));
        h_buckets.at(partition_ids.first)[partition_ids.second].inc_size();
    }
    
    double sr = success_count / node_count;
    if (new_partition && tau > (sr+0.0001)) {
        tau = (tau-sr);
        success_count+=1;
    } else if (tau < tau_limit) {
        double fr = 1-(sr); 
        tau += fr;
    }
    node_count+=1;


}

InterBiasedProgressPolicy::PartitionNode InterBiasedProgressPolicy::remove_partition(int partition_key) {
    auto partition_ids = partition_to_id_pair.at(partition_key); 
    auto partition = utils::swap_and_pop_from_vector(h_buckets.at(partition_ids.first), partition_ids.second);

    auto& p_from_back = h_buckets.at(partition_ids.first)[partition_ids.second];
    partition_to_id_pair.at(p_from_back.partition).second = partition_ids.second;

    if (h_buckets.at(partition_ids.first).empty()) {
        h_buckets.erase(partition_ids.first);
        if (ignore_size) {
            current_sum -= std::exp(static_cast<double>(partition_ids.first) / tau);
        }
    }
    if (!ignore_size) {
        current_sum -= std::exp(static_cast<double>(partition_ids.first) / tau);
    }

    partition_to_id_pair.erase(partition_key);
    return partition;
}

void InterBiasedProgressPolicy::notify_removal(int partition_key, int node_key) {
    auto& partition_ids = partition_to_id_pair.at(partition_key);
    auto &last_partition_dfn = h_buckets.at(partition_ids.first)[partition_ids.second];
    last_partition_dfn.dec_size();
    if (last_partition_dfn.size == 0) {
        remove_partition(partition_key);
    }
    node_to_prog.erase(node_key);
}


class InterBiasedProgressPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterBiasedProgressPolicy> {
public:
    InterBiasedProgressPolicyFeature() : TypedFeature("inter_biased_depth") {
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
        utils::add_rng_options(*this);
        add_partition_policy_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterBiasedProgressPolicyFeature> _plugin;
}