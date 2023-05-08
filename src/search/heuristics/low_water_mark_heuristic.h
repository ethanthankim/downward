#ifndef HEURISTICS_LWM_HEURISTIC_H
#define HEURISTICS_LWM_HEURISTIC_H

#include "relaxation_heuristic.h"

#include <cassert>

namespace low_water_mark_heuristic {

class LowWaterMarkHeuristic : public relaxation_heuristic::RelaxationHeuristic {
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LowWaterMarkHeuristic(const plugins::Options &opts);
};
}

#endif
