H="hff=ff(transform=adapt_costs(cost_type=one))"
FOLDER="smoke_test"

# echo "testing barman"
# ./fast-downward.py benchmarks-test/barman-sat14-strips/domain.pddl benchmarks-test/barman-sat14-strips/p4-11-4-14.pddl --evaluator "$H" --search "eager($1, cost_type=one)" > $FOLDER/barman.txt 2>&1

# echo "testing elevators"
# ./fast-downward.py benchmarks-test/elevators-sat11-strips/domain.pddl benchmarks-test/elevators-sat11-strips/p19.pddl --evaluator "$H" --search "eager($1, cost_type=one)"

echo "testing pegsol"
./fast-downward.py benchmarks-test/pegsol-sat11-strips/domain.pddl benchmarks-test/pegsol-sat11-strips/p13.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/pegsol.txt 2>&1

echo "testing sokoban"
./fast-downward.py benchmarks-test/sokoban-sat11-strips/domain.pddl benchmarks-test/sokoban-sat11-strips/p07.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/sokoban.txt 2>&1

# echo "testing tetris"
# ./fast-downward.py benchmarks-test/tetris-sat14-strips/domain.pddl benchmarks-test/tetris-sat14-strips/p033.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> bulk_test/tetris.txt 2>&1

# echo "testing transport"
# ./fast-downward.py benchmarks-test/transport-sat11-strips/domain.pddl benchmarks-test/transport-sat11-strips/p18.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> bulk_test/transport.txt 2>&1

echo "testing tidybot"
./fast-downward.py benchmarks-test/tidybot-sat11-strips/domain.pddl benchmarks-test/tidybot-sat11-strips/p14.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/tidybot.txt 2>&1

# echo "testing nomystery"
# ./fast-downward.py benchmarks-test/nomystery-sat11-strips/domain.pddl benchmarks-test/nomystery-sat11-strips/p16.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/nomystery.txt 2>&1

echo "testing floortile"
./fast-downward.py benchmarks-test/floortile-sat11-strips/domain.pddl benchmarks-test/floortile-sat11-strips/seq-p01-002.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/floortile.txt 2>&1

# echo "testing scanalyzer"
# ./fast-downward.py benchmarks-test/scanalyzer-sat11-strips/domain.pddl benchmarks-test/scanalyzer-sat11-strips/p20.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/scanalyzer.txt 2>&1

echo "testing ged"
./fast-downward.py benchmarks-test/ged-sat14-strips/domain.pddl benchmarks-test/ged-sat14-strips/d-10-3.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/ged.txt 2>&1
