#include "partition_policy.h"

using namespace std;

PartitionPolicy::PartitionPolicy(const plugins::Options &opts)
    : description(opts.get_unparsed_config()),
      log(utils::get_log_from_options(opts)) {
}

const string &PartitionPolicy::get_description() const {
    return description;
}

void add_partition_policy_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}



static class PartitionPolicyCategoryPlugin : public plugins::TypedCategoryPlugin<PartitionPolicy> {
public:
    PartitionPolicyCategoryPlugin() : TypedCategoryPlugin("Partition Policies") {
        document_synopsis(
            "Partition selection policies (inter)"
        );
        allow_variable_binding();
    }
}
_category_plugin;