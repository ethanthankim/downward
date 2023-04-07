#include "eager_search_anytime.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_eager_anytime {
class EagerSearchAnytimeFeature : public plugins::TypedFeature<SearchEngine, eager_search_anytime::EagerSearchAnytime> {
public:
    EagerSearchAnytimeFeature() : TypedFeature("eager_anytime") {
        document_title("Eager anytime best-first search");
        document_synopsis("");

        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        // add_list_option<shared_ptr<Evaluator>>(
        //     "evaluators",
        //     "Evaluator schedule used for anytime search improved solutions.");
        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        add_option<shared_ptr<Evaluator>>(
            "f_eval",
            "set evaluator for jump statistics. "
            "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
            plugins::ArgumentInfo::NO_DEFAULT);
        eager_search_anytime::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<EagerSearchAnytimeFeature> _plugin;
}
