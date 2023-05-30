#ifndef OPEN_LISTS_LINEAR_WEIGHTED_OPEN_LIST_H
#define OPEN_LISTS_LINEAR_WEIGHTED_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../plugins/plugin.h"


namespace linear_weighted_open_list {
class LinearWeightedOpenListFactory : public OpenListFactory {
  plugins::Options options;

 public:
  explicit LinearWeightedOpenListFactory(const plugins::Options &options);
  virtual ~LinearWeightedOpenListFactory() override = default;

  virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
  virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}  

#endif


