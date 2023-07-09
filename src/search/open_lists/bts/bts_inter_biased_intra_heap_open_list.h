#ifndef OPEN_LISTS_INTER_BIASED_INTRA_HEAP_OPEN_LIST_H
#define OPEN_LISTS_INTER_BIASED_INTRA_HEAP_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace inter_biased_intra_heap_open_list {
class BTSInterBiasedIntraHeapOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSInterBiasedIntraHeapOpenListFactory(const plugins::Options &options);
    virtual ~BTSInterBiasedIntraHeapOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif