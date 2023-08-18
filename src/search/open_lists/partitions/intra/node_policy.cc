#include "node_policy.h"

using namespace std;

NodePolicy::NodePolicy(const plugins::Options &opts)
    : description(opts.get_unparsed_config()),
      log(utils::get_log_from_options(opts)) {
}

const string &NodePolicy::get_description() const {
    return description;
}

void add_node_policy_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}



static class NodePolicyCategoryPlugin : public plugins::TypedCategoryPlugin<NodePolicy> {
public:
    NodePolicyCategoryPlugin() : TypedCategoryPlugin("Node Policies") {
        document_synopsis(
            "Node selection policies (intra)"
        );
        allow_variable_binding();
    }
}
_category_plugin;