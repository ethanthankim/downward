#include "biased_minh.h"

using namespace std;

namespace inter_biased_minh_partition {

InterBiasedMinHPolicy::InterBiasedMinHPolicy(const plugins::Options &opts)
    : PartitionPolicy(opts),
    evaluator(opts.get<shared_ptr<Evaluator>>("eval")),
    rng(utils::parse_rng_from_options(opts)),
    tau(opts.get<double>("tau")),
    ignore_size(opts.get<bool>("ignore_size")),
    ignore_weights(opts.get<bool>("ignore_weights")),
    relative_h(opts.get<bool>("relative_h")),
    relative_h_offset(opts.get<int>("relative_h_offset")),
    current_sum(0.0) {}

void InterBiasedMinHPolicy::insert_partition(int new_h, PartitionNode &partition) {
    bool h_absent = h_buckets.find(new_h) == h_buckets.end();
    if (ignore_size) {
        if (h_absent) {
            if (ignore_weights)
                current_sum += 1;
            else if (!relative_h)
                current_sum += std::exp(-1.0 * static_cast<double>(new_h) / tau);
        }
    } else {
        if (ignore_weights)
            current_sum += 1;
        else if (!relative_h)
            current_sum += std::exp(-1.0 * static_cast<double>(new_h) / tau);
    }
    h_buckets[new_h].push_back(partition);
}

void InterBiasedMinHPolicy::remove_last_partition() {
    utils::swap_and_pop_from_vector(h_buckets.at(last_chosen_key), last_chosen_partition_i);
    if (h_buckets.at(last_chosen_key).empty()) {
        h_buckets.erase(last_chosen_key);
        if (ignore_size) {
            if (ignore_weights)
                current_sum -= 1;
            else if (!relative_h)
                current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_key) / tau);
        }
    }
    if (!ignore_size) {
        if (ignore_weights)
            current_sum -= 1;
        else if (!relative_h)
            current_sum -= std::exp(-1.0 * static_cast<double>(last_chosen_key) / tau);
    }
}

bool InterBiasedMinHPolicy::maybe_move_last_partition() {

    if (h_buckets.at(last_chosen_key)[last_chosen_partition_i].state_hs.at(last_chosen_h) == 0)
        h_buckets.at(last_chosen_key)[last_chosen_partition_i].state_hs.erase(last_chosen_h);

    auto last_partition = h_buckets.at(last_chosen_key)[last_chosen_partition_i];
    int new_h = last_partition.state_hs.begin()->first;
    if (new_h != last_chosen_key) {

        remove_last_partition();
        insert_partition(new_h, last_partition);

        return true;
    }  
    return false;
}

int InterBiasedMinHPolicy::get_next_partition() { 

    if (last_chosen_key != -1) { // gross
        auto last_partition_dfn = h_buckets.at(last_chosen_key)[last_chosen_partition_i];
        if (last_partition_dfn.state_hs.begin()->second == 0 
                && last_partition_dfn.state_hs.size() == 1) { 
            remove_last_partition();
        } else {
            maybe_move_last_partition();
        }
    }

    int key = h_buckets.begin()->first;
    if (h_buckets.size() > 1) {
        double r = rng->random();
        if (relative_h) {
            double total_sum = 0;
            int i = relative_h_offset;
            for (auto it : h_buckets) {
                double s = std::exp(-1.0 * static_cast<double>(i) / tau);
                if (!ignore_size) s *= static_cast<double>(it.second.size());
                total_sum += s;
                ++i;
            }
            double p_sum = 0.0;
            i = relative_h_offset;
            for (auto it : h_buckets) {
                double p = std::exp(-1.0 * static_cast<double>(i) / tau) / total_sum;
                if (!ignore_size) p *= static_cast<double>(it.second.size());
                p_sum += p;
                ++i;
                if (r <= p_sum) {
                    key = it.first;
                    break;
                }
            }
        } else {
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
                    key = value;
                    break;
                }
            }
        }
    }

    vector<PartitionNode> &partitions = h_buckets[key];
    assert(!partitions.empty());

    last_chosen_key = key;
    last_chosen_partition_i = rng->random(partitions.size());
    last_chosen_partition_key = partitions[last_chosen_partition_i].partition;

    return last_chosen_partition_key;
}

void InterBiasedMinHPolicy::notify_insert(
        int partition_key,
        int node_key,
        bool new_partition,
        EvaluationContext &eval_context) 
{
    int key = eval_context.get_evaluator_value(evaluator.get());
    if (new_partition) {
        auto new_partition = PartitionNode(
            partition_key,
            map<int, int>{{key, 1}}
        );
        insert_partition(key, new_partition);
    } else {
        auto &counts = h_buckets.at(last_chosen_key)[last_chosen_partition_i].state_hs;
        counts[key] += 1;
    }
    node_hs.emplace(node_key, key);
}

void InterBiasedMinHPolicy::notify_removal(int partition_key, int node_key) {

    last_chosen_h = node_hs[node_key];
    auto &last_partition_dfn = h_buckets.at(last_chosen_key)[last_chosen_partition_i];
    last_partition_dfn.state_hs.at(last_chosen_h) -= 1;
    node_hs.erase(node_key);

}

class InterBiasedMinHPolicyFeature : public plugins::TypedFeature<PartitionPolicy, InterBiasedMinHPolicy> {
public:
    InterBiasedMinHPolicyFeature() : TypedFeature("inter_biased_minh") {
        document_subcategory("partition_policies");
        document_title("Biased partition selection");
        document_synopsis(
            "Choose the next partition biased in favour of low h.");
        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
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