#include "partition_system.h"

using namespace std;

PartitionSystem::PartitionSystem(const plugins::Options &opts)
    : description(opts.get_unparsed_config()),
      log(utils::get_log_from_options(opts)) {
}

const string &PartitionSystem::get_description() const {
    return description;
}

bool PartitionSystem::safe_to_remove_node(Key to_remove) {
    return true;
}

bool PartitionSystem::safe_to_remove_partition(Key to_remove) {
    return true;
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