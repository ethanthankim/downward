#include "../../../plugins/plugin.h"

namespace node_policy_plugin_group {
static class NodePolicyGroupPlugin : public plugins::SubcategoryPlugin {
public:
    NodePolicyGroupPlugin() : SubcategoryPlugin("node_policies") {
        document_title("Node Policies");
    }
}
_subcategory_plugin;
}
