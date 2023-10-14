# echo "testing barman"
# ./fast-downward.py benchmarks-test/barman-sat14-strips/domain.pddl benchmarks-test/barman-sat14-strips/p4-11-4-14.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))" > barman.txt 2>&1

# echo "testing elevators"
# ./fast-downward.py benchmarks-test/elevators-sat11-strips/domain.pddl benchmarks-test/elevators-sat11-strips/p19.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"

# echo "testing pegsol"
# ./fast-downward.py benchmarks-test/pegsol-sat11-strips/domain.pddl benchmarks-test/pegsol-sat11-strips/p13.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> pegsol.txt 2>&1

# echo "testing sokoban"
# ./fast-downward.py benchmarks-test/sokoban-sat11-strips/domain.pddl benchmarks-test/sokoban-sat11-strips/p07.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> sokoban.txt 2>&1

# echo "testing tetris"
# ./fast-downward.py benchmarks-test/tetris-sat14-strips/domain.pddl benchmarks-test/tetris-sat14-strips/p033.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> tetris.txt 2>&1

# echo "testing transport"
# ./fast-downward.py benchmarks-test/transport-sat11-strips/domain.pddl benchmarks-test/transport-sat11-strips/p18.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> transport.txt 2>&1

echo "testing tidybot"
./fast-downward.py benchmarks-test/tidybot-sat11-strips/domain.pddl benchmarks-test/tidybot-sat11-strips/p14.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> tidybot.txt 2>&1

echo "testing nomystery"
./fast-downward.py benchmarks-test/nomystery-sat11-strips/domain.pddl benchmarks-test/nomystery-sat11-strips/p16.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> nomystery.txt 2>&1

echo "testing floortile"
./fast-downward.py benchmarks-test/floortile-sat11-strips/domain.pddl benchmarks-test/floortile-sat11-strips/seq-p01-002.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> floortile.txt 2>&1

echo "testing scanalyzer"
./fast-downward.py benchmarks-test/scanalyzer-sat11-strips/domain.pddl benchmarks-test/scanalyzer-sat11-strips/p20.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> scanalyzer.txt 2>&1

echo "testing ged"
./fast-downward.py benchmarks-test/ged-sat14-strips/domain.pddl benchmarks-test/ged-sat14-strips/d-10-3.pddl --evaluator "hff=ff()" --search "eager(hi_partition(hff, inter_biased_minh(hff, ignore_size=true, random_seed=2), intra_ep_minh(hff, 0.2, random_seed=4) ))"> ged.txt 2>&1