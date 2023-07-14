#ifndef OPEN_LISTS_INTER_BIASED_INTRA_EPSILON_OPEN_LIST_H
#define OPEN_LISTS_INTER_BIASED_INTRA_EPSILON_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace inter_biased_intra_epsilon {
class BTSInterBiasedIntraEpOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSInterBiasedIntraEpOpenListFactory(const plugins::Options &options);
    virtual ~BTSInterBiasedIntraEpOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif