#include "../plugins/plugin.h"

namespace partition_system_plugin_group {
static class PartitionSystemGroupPlugin : public plugins::SubcategoryPlugin {
public:
    PartitionSystemGroupPlugin() : SubcategoryPlugin("partition_systems") {
        document_title("Partition Systems");
    }
}
_subcategory_plugin;
}
