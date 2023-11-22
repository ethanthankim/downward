#include "new_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"
#include "../utils/rng_options.h"

using namespace std;

namespace plugin_new_search {
class NewSearchFeature : public plugins::TypedFeature<SearchEngine, new_search::NewSearch> {
public:
    NewSearchFeature() : TypedFeature("new_search") {
        document_title("search");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        // add_list_option<shared_ptr<Evaluator>>(
        //     "preferred",
        //     "use preferred operators of these evaluators",
        //     "[]");
        new_search::add_options_to_feature(*this);
        utils::add_rng_options(*this); 
    }
};

static plugins::FeaturePlugin<NewSearchFeature> _plugin;
}
