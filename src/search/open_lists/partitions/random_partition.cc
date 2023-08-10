#include "random_partition.h"

using namespace std;

namespace random_partition {

RandomPartition::RandomPartition(const plugins::Options &opts)
    : PartitionSystem(opts), 
      rng(utils::parse_rng_from_options(opts)),
      epsilon(opts.get<double>("epsilon"))  {}

Key RandomPartition::insert_state(Key to_insert, const utils::HashMap<Key, OpenState> &open_states) {   

    Key random_key;
    if ( (rng->random() < epsilon) || partitions.empty()) {
        // new type
        random_key = to_insert;
        partitions.emplace(random_key, vector<Key>{to_insert});
    }

    else {
        // existing type
        auto it = partitions.begin();
        std::advance(it, rng->random(partitions.size()));
        random_key = it->first;
        vector<Key> &partition = partitions.at(random_key);
        partition.push_back(to_insert);
    }

    return random_key;
}

Key RandomPartition::select_next_partition(const utils::HashMap<Key, OpenState> &open_states) {
    if (last_partition != -1 && partitions.at(last_partition).empty())
        partitions.erase(last_partition);

    auto it = partitions.begin();
    std::advance(it, rng->random(partitions.size()));
    return it->first;
}

Key RandomPartition::select_next_state_from_partition(Key partition_key, const utils::HashMap<Key, OpenState> &open_states) {
    vector<Key> &partition = partitions.at(partition_key);

    int to_remove_i = rng->random(partition.size());
    Key to_remove = utils::swap_and_pop_from_vector(partition, to_remove_i);

    last_partition = partition_key;
    return to_remove;
}

class RandomPartitionFeature : public plugins::TypedFeature<PartitionSystem, RandomPartition> {
public:
    RandomPartitionFeature() : TypedFeature("random_partition") {
        document_subcategory("partition_systems");
        document_title("random partitioning");
        document_synopsis(
            "With probability epsilon, put node in random partition, otherwise put it in its own.");
        add_option<double>(
            "epsilon",
            "probability of partitioning a node randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options(*this);
        add_partition_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<RandomPartitionFeature> _plugin;
}