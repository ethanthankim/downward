H="hff=ff(transform=adapt_costs(cost_type=one))"
FOLDER="smoke_test"

echo "testing pegsol"
./fast-downward.py benchmarks-test/pegsol-sat11-strips/domain.pddl benchmarks-test/pegsol-sat11-strips/p13.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/pegsol.txt 2>&1
grep Expanded "$FOLDER/pegsol.txt"
echo

echo "testing sokoban"
./fast-downward.py benchmarks-test/sokoban-sat11-strips/domain.pddl benchmarks-test/sokoban-sat11-strips/p07.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/sokoban.txt 2>&1
grep Expanded "$FOLDER/sokoban.txt"
echo

echo "testing tidybot"
./fast-downward.py benchmarks-test/tidybot-sat11-strips/domain.pddl benchmarks-test/tidybot-sat11-strips/p14.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/tidybot.txt 2>&1
grep Expanded "$FOLDER/tidybot.txt"
echo

echo "tsting floortile"
./fast-downward.py benchmarks-test/floortile-sat11-strips/domain.pddl benchmarks-test/floortile-sat11-strips/seq-p01-002.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/floortile.txt 2>&1
grep Expanded "$FOLDER/floortile.txt"
echo

echo "testing ged"
./fast-downward.py benchmarks-test/ged-sat14-strips/domain.pddl benchmarks-test/ged-sat14-strips/d-10-3.pddl --evaluator "$H" --search "eager($1, cost_type=one)"> $FOLDER/ged.txt 2>&1
grep Expanded "$FOLDER/ged.txt"
echo