H="hff=ff(transform=adapt_costs(cost_type=one))"
# H="hff=ff()"
FOLDER="smoke_test"
COST="one"
# COST="normal"

echo "testing pegsol"
./fast-downward.py --validate benchmarks-test/pegsol-sat11-strips/domain.pddl benchmarks-test/pegsol-sat11-strips/p13.pddl --evaluator "$H" --search "eager($1, cost_type=$COST)"> $FOLDER/pegsol.txt 2>&1
grep Expanded "$FOLDER/pegsol.txt"
echo

echo "testing sokoban"
./fast-downward.py --validate benchmarks-test/sokoban-sat11-strips/domain.pddl benchmarks-test/sokoban-sat11-strips/p07.pddl --evaluator "$H" --search "eager($1, cost_type=$COST)"> $FOLDER/sokoban.txt 2>&1
grep Expanded "$FOLDER/sokoban.txt"
echo

echo "testing tidybot"
./fast-downward.py --validate benchmarks-test/tidybot-sat11-strips/domain.pddl benchmarks-test/tidybot-sat11-strips/p14.pddl --evaluator "$H" --search "eager($1, cost_type=$COST)"> $FOLDER/tidybot.txt 2>&1
grep Expanded "$FOLDER/tidybot.txt"
echo

echo "testing floortile"
./fast-downward.py --validate benchmarks-test/floortile-sat11-strips/domain.pddl benchmarks-test/floortile-sat11-strips/seq-p01-002.pddl --evaluator "$H" --search "eager($1, cost_type=$COST)"> $FOLDER/floortile.txt 2>&1
grep Expanded "$FOLDER/floortile.txt"
echo

echo "testing ged"
./fast-downward.py --validate benchmarks-test/ged-sat14-strips/domain.pddl benchmarks-test/ged-sat14-strips/d-10-3.pddl --evaluator "$H" --search "eager($1, cost_type=$COST)"> $FOLDER/ged.txt 2>&1
grep Expanded "$FOLDER/ged.txt"
echo