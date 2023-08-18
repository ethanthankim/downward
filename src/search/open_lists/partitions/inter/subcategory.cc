#include "../../../plugins/plugin.h"

namespace partition_policy_plugin_group {
static class PartitionPolicyGroupPlugin : public plugins::SubcategoryPlugin {
public:
    PartitionPolicyGroupPlugin() : SubcategoryPlugin("partition_policies") {
        document_title("Partition Policies");
    }
}
_subcategory_plugin;
}
