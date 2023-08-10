#ifndef PARTITIONS_HI_PARTITION_H
#define PARTITIONS_HI_PARTITION_H

#include "partition_system.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace hi_partition {
class HIPartition : public PartitionSystem {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    double inter_epsilon, intra_epsilon;

    utils::HashMap<Key, vector<Key>> heaped_partitions;
    vector<Key> partition_heap;
    int last_partition_index = -1;
    bool last_partition_was_emptied = false;

private:
    using Comparer = bool (HIPartition::*)(vector<Key>&, int, int, const utils::HashMap<Key, OpenState> &);
    void adjust_heap_down(vector<Key> &heap, int loc, Comparer, const utils::HashMap<Key, OpenState> &open_states); 
    void adjust_heap_up(vector<Key> &heap, int loc, Comparer, const utils::HashMap<Key, OpenState> &open_states);   
    void adjust_to_top(vector<Key> &heap, int loc); 
    Key random_access_heap_pop(vector<Key> &heap, int loc, Comparer, const utils::HashMap<Key, OpenState> &open_states);

    bool compare_parent_node_smaller(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states);
    bool compare_parent_node_bigger(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states);
    bool compare_parent_type_smaller(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states);
    bool compare_parent_type_bigger(vector<Key>& heap, int parent, int child, const utils::HashMap<Key, OpenState> &open_states);
public:
    explicit HIPartition(const plugins::Options &opts);
    virtual ~HIPartition() override = default;

    virtual Key insert_state(Key to_insert, const utils::HashMap<Key, OpenState> &open_states) override;
    virtual Key select_next_partition(const utils::HashMap<Key, OpenState> &open_states) override;
    virtual Key select_next_state_from_partition(Key partition, const utils::HashMap<Key, OpenState> &open_states) override;

};
}

#endif