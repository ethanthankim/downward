#ifndef OPEN_LISTS_INTRA_PERCOLATION_OPEN_LIST_H
#define OPEN_LISTS_INTRA_PERCOLATION_OPEN_LIST_H

#include "../../open_list_factory.h"

#include "../../plugins/plugin.h"


/*
  Bench Transition System Entry State Open List. 
*/

namespace type_intra_percolation_open_list {
class BTSIntraPercolationOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit BTSIntraPercolationOpenListFactory(const plugins::Options &options);
    virtual ~BTSIntraPercolationOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif