#include "lazy_search_anytime.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy_anytime {
class LazySearchAnytimeFeature : public plugins::TypedFeature<SearchEngine, lazy_search_anytime::LazySearchAnytime> {
public:
    LazySearchAnytimeFeature() : TypedFeature("lazy_anytime") {
        document_title("Lazy anytime best-first search");
        document_synopsis("");

        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        // add_list_option<int>(
        //     "weights",
        //     "Evaluator weight schedule. Go to the next weight whenever a new improved solution is found");
        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        add_option<shared_ptr<Evaluator>>(
            "f_eval",
            "set evaluator for jump statistics. "
            "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
            plugins::ArgumentInfo::NO_DEFAULT);
        lazy_search_anytime::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<LazySearchAnytimeFeature> _plugin;
}
