#include "partition_system.h"

using namespace std;

PartitionSystem::PartitionSystem(const plugins::Options &opts)
    : description(opts.get_unparsed_config()),
      log(utils::get_log_from_options(opts)) {
}

const string &PartitionSystem::get_description() const {
    return description;
}

pair<bool, PartitionKey> PartitionSystem::choose_state_partition(utils::HashMap<NodeKey, PartitionedState> active_states) {
    return make_pair(false, 0); // default to trivial case where every node gets put in one partition
}

void add_partition_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}



static class PartitionSystemCategoryPlugin : public plugins::TypedCategoryPlugin<PartitionSystem> {
public:
    PartitionSystemCategoryPlugin() : TypedCategoryPlugin("Partition") {
        document_synopsis(
            "TODO"
        );
        allow_variable_binding();
    }
}
_category_plugin;