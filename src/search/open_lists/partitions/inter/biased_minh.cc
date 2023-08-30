#include "biased_minh.h"

using namespace std;

namespace inter_biased_partition {

InterBiasedPolicy::InterBiasedPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    rng(utils::parse_rng_from_options(opts)),
    tau(opts.get<double>("tau")),
    ignore_size(opts.get<bool>("ignore_size")),
    ignore_weights(opts.get<bool>("ignore_weights")),
    current_sum(0.0) {}


PartitionKey InterBiasedPolicy::get_next_partition(utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) { 
    
    if ( (partition_buckets.find(last_chosen_partition_key) != partition_buckets.end()) 
        && partition_buckets.at(last_chosen_partition_key).empty() ) {
        
        utils::swap_and_pop_from_vector(h_buckets.at(last_chosen_h), last_chosen_partition_i);
        partition_buckets.erase(last_chosen_partition_key);

        vector<PartitionKey>& h_partitions = h_buckets.at(last_chosen_h);
        if (h_partitions.empty()) {
            h_buckets.erase(last_chosen_h);

            if (ignore_size) {
                if (ignore_weights)
                    current_sum -= 1;
                else if (!relative_h)
                    current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_h) / tau);
            }
        }

        if (!ignore_size) {
            if (ignore_weights) {
                current_sum -= 1;
            } else {
                current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_h) / tau);
            }
        }
    }
    
    
    int key = (*h_buckets.begin()).first;
    if (h_buckets.size() > 1) {
        double r = rng->random();
        double p_sum = 0.0;
        
        for (auto bucket_pair : h_buckets) {
            auto value = bucket_pair.first;
            double p =  1.0 / current_sum;

            if (!ignore_weights)
                p *= std::exp(-1.0 * static_cast<double>(value) / tau);

            if (!ignore_size)
                p *= static_cast<double>(h_buckets[value].size()); 

            p_sum += p;
            if (r <= p_sum) {
                key = value;
                break;
            }
        }
    }
    vector<PartitionKey> &partition_keys = h_buckets[key];
    assert(!partition_keys.empty());

    last_chosen_h = key;
    last_chosen_partition_i = rng->random(partition_keys.size());
    last_chosen_partition_key = partition_keys[last_chosen_partition_i];

    return last_chosen_partition_key;
}

void InterBiasedPolicy::notify_insert(bool new_type, NodeKey inserted, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) {
    
    int key = active_states.at(inserted).h;
    if (new_type) {
        PartitionKey partition = active_states.at(inserted).partition;

        h_buckets[key].push_back(partition);

        if (ignore_size) {
            if (ignore_weights)
                current_sum += 1;
            else if (!relative_h)
                current_sum += std::exp(-1.0 * static_cast<double>(key) / tau);
        }
    }

    if (!ignore_size) {
        if (ignore_weights) {
            current_sum += 1;
        } else {
            current_sum += std::exp(-1.0 * static_cast<double>(key) / tau);
        }
    }
}

void InterBiasedPolicy::notify_remove(NodeKey removed, utils::HashMap<NodeKey, PartitionedState> &active_states, utils::HashMap<PartitionKey, Partition> &partition_buckets) {

    assert(active_states.at(removed).partition == last_chosen_partition_key);
    assert(h_buckets.at(last_chosen_h)[last_chosen_partition_i] == last_chosen_partition_key);

    //theres an assumption here that the partition_buckets are stored as a heap and the 0th element is the minh element.
    int new_partition_h = active_states.at(
        partition_buckets.at(last_chosen_partition_key)[0]
    ).h;
    if (new_partition_h != last_chosen_h) {
        // Below implies the need for a partition mover function that will move a partition to a new h_bucket and make all the necessary softmin changes
        
        //remove from h_bucket
        utils::swap_and_pop_from_vector(h_buckets.at(last_chosen_h), last_chosen_partition_i);
        if (h_buckets.at(last_chosen_h).empty()) {
            h_buckets.erase(last_chosen_h);
            if (ignore_size) {
                if (ignore_weights)
                    current_sum -= 1;
                else if (!relative_h)
                    current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_h) / tau);
            }
        }
        if (!ignore_size) {
            if (ignore_weights)
                current_sum -= 1;
            else if (!relative_h)
                current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_h) / tau);
        }

        //insert to h_bucket
        if (ignore_size) {
            if (h_buckets.find(new_partition_h) == h_buckets.end()) {
                if (ignore_weights)
                    current_sum += 1;
                else if (!relative_h)
                    current_sum += std::exp(-1.0 * static_cast<double>(new_partition_h) / tau);
            }
        } else {
            if (ignore_weights)
                current_sum += 1;
            else if (!relative_h)
                current_sum += std::exp(-1.0 * static_cast<double>(new_partition_h) / tau);

        }
        h_buckets[new_partition_h].push_back(last_chosen_partition_key);

        // keep things synced!
        last_chosen_h = new_partition_h;
        last_chosen_partition_i = h_buckets[new_partition_h].size()-1;

    }

}

class InterBiasedPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterBiasedPolicy> {
public:
    InterBiasedPolicyFeature() : TypedFeature("inter_biased") {
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
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<InterBiasedPolicyFeature> _plugin;
}