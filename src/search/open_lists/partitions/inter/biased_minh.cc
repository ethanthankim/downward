#include "biased_minh.h"

#include "../utils/collections.h"
#include "../utils/hash.h"

using namespace std;

namespace inter_biased_minh_partition {

InterBiasedMinHPolicy::InterBiasedMinHPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    tau(opts.get<double>("tau")),
    ignore_size(opts.get<bool>("ignore_size")),
    ignore_weights(opts.get<bool>("ignore_weights")),
    current_sum(0.0) {}

// void InterBiasedMinHPolicy::verify_heap() {

//     bool valid = true;
//     for (auto &b : h_buckets) {
//         int mini = b.first;
//         int i = 0;
//         for (auto &p: b.second) {
//             auto &p_ids = partition_to_id_pair.at(p.partition);
//             if (p.h_counts.begin()->first != mini) {
//                 valid = false;
//             } 
//             if (p_ids.first != mini) {
//                 valid = false;
//             }
//             if (p_ids.second != i) {
//                 valid = false;
//             }

//             if (!valid) {
//                 cout << "Ahhh the heap isn't valid... mini:" <<  mini 
//                     << ", p.h_counts.begin()->first:" << p.h_counts.begin()->first
//                     << ", first:" 
//                     << p_ids.first 
//                     << ", second:" << p_ids.second 
//                     << ", i:" << i;
//                 exit(1);
//             }
//             i+=1;
//         }
//     }

// }


void InterBiasedMinHPolicy::insert_partition(int new_h, PartitionNode &partition) {
    bool h_absent = h_buckets.find(new_h) == h_buckets.end();
    if (ignore_size) {
        if (h_absent) {
            if (ignore_weights)
                current_sum += 1;
            else
                current_sum += std::exp(-1.0 * static_cast<double>(new_h) / tau);
        }
    } else {
        if (ignore_weights)
            current_sum += 1;
        else
            current_sum += std::exp(-1.0 * static_cast<double>(new_h) / tau);
    }
    h_buckets[new_h].push_back(partition);
    partition_to_id_pair.emplace(partition.partition, make_pair(new_h, h_buckets[new_h].size()-1)) ;
}

void InterBiasedMinHPolicy::modify_partition(int partition_key, int h) {
    auto partition_ids = partition_to_id_pair.at(partition_key); 
    h_buckets.at(partition_ids.first)[partition_ids.second].inc_h_count(h);
}

InterBiasedMinHPolicy::PartitionNode InterBiasedMinHPolicy::remove_partition(int partition_key) {
    auto partition_ids = partition_to_id_pair.at(partition_key); 
    auto partition = utils::swap_and_pop_from_vector(h_buckets.at(partition_ids.first), partition_ids.second);

    auto& p_from_back = h_buckets.at(partition_ids.first)[partition_ids.second];
    partition_to_id_pair.at(p_from_back.partition).second = partition_ids.second;

    if (h_buckets.at(partition_ids.first).empty()) {
        h_buckets.erase(partition_ids.first);
        if (ignore_size) {
            if (ignore_weights)
                current_sum -= 1;
            else
                current_sum -= std::exp(-1.0 * static_cast<double>(partition_ids.first) / tau);
        }
    }
    if (!ignore_size) {
        if (ignore_weights)
            current_sum -= 1;
        else
            current_sum -= std::exp(-1.0 * static_cast<double>(partition_ids.first) / tau);
    }

    partition_to_id_pair.erase(partition_key);
    return partition;
}

bool InterBiasedMinHPolicy::maybe_move_partition(int partition_key) {

    auto& partition_ids = partition_to_id_pair.at(partition_key); 
    int true_min_h = h_buckets.at(partition_ids.first)[partition_ids.second].h_counts.begin()->first;
    if (true_min_h != partition_ids.first) {
        auto partition = remove_partition(partition_key);
        insert_partition(true_min_h, partition);

        return true;

    } 
    return false;

}

int InterBiasedMinHPolicy::get_next_partition() { 

    // total_gets+=1;
    // if (total_gets % 100 == 0) verify_heap();


    int selected_h = h_buckets.begin()->first;
    // int count_i = 0;
    if (h_buckets.size() > 1) {
        double r = rng->random();
        double p_sum = 0.0;
        for (auto bucket_pair : h_buckets) {
            auto value = bucket_pair.first;
            double p =  1.0 / current_sum;

            if (!ignore_weights)
                p *= std::exp(-1.0 * static_cast<double>(value) / tau);

            if (!ignore_size)
                p *= static_cast<double>(h_buckets.at(value).size()); 

            p_sum += p;
            if (r <= p_sum) {
                selected_h = value;
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
    // }

    vector<PartitionNode> &partitions = h_buckets[selected_h];
    assert(!partitions.empty());

    return  rng->choose(partitions)->partition;

}

void InterBiasedMinHPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        int eval) 
{
    node_hs.emplace(node_key, eval);
    if (new_partition || partition_to_id_pair.count(partition_key)==0) {
        auto new_partition = PartitionNode(
            partition_key,
            map<int, int>{{eval, 1}}
        );
        insert_partition(eval, new_partition);
    } else {
        modify_partition(partition_key, eval);
    }
    maybe_move_partition(partition_key);
}

void InterBiasedMinHPolicy::notify_removal(int partition_key, int node_key) {
    auto& partition_ids = partition_to_id_pair.at(partition_key);

    int removed_h = node_hs[node_key];
    auto &last_partition_dfn = h_buckets.at(partition_ids.first)[partition_ids.second];
    node_hs.erase(node_key);

    last_partition_dfn.dec_h_count(removed_h);
    if (last_partition_dfn.h_counts.empty()) {
        remove_partition(partition_key);
    } else {
        maybe_move_partition(partition_key);
    }

}

class InterBiasedMinHPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterBiasedMinHPolicy> {
public:
    InterBiasedMinHPolicyFeature() : TypedFeature("inter_biased_minh") {
        document_subcategory("partition_policies");
        document_title("Biased partition selection");
        document_synopsis(
            "Choose the next partition biased in favour of low h.");
        add_option<double>(
            "tau",
            "temperature parameter of softmin", "1.0");
        add_option<bool>(
            "ignore_size",
            "ignore bucket sizes", "false");
        add_option<bool>(
            "ignore_weights",
            "ignore linear_weighted weights", "false");
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

static plugins::FeaturePlugin<InterBiasedMinHPolicyFeature> _plugin;
}