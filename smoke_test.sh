# H="h=ff(transform=adapt_costs(cost_type=one))"
HFF="hff=ff()"
H="h=lwm(ff())"
FOLDER="smoke_test"
COST="one"
EVALS="--evaluator $H --evaluator $HFF"
# COST="normal"

echo "testing pegsol"
./fast-downward.py --validate benchmarks-test/pegsol-sat11-strips/domain.pddl benchmarks-test/pegsol-sat11-strips/p13.pddl $EVALS --search --if-unit-cost "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=true), let(hff, ff(), lazy_greedy([hff,hlm],preferred=[hff,hlm]) ))" --if-non-unit-cost "let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=true), let(hff1, ff(transform=adapt_costs(one)), lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1], cost_type=one,reopen_closed=false) ))"> $FOLDER/pegsol.txt 2>&1
grep Expanded "$FOLDER/pegsol.txt"
grep -i "Plan valid" "$FOLDER/pegsol.txt"
echo

echo "testing sokoban"
./fast-downward.py --validate benchmarks-test/sokoban-sat11-strips/domain.pddl benchmarks-test/sokoban-sat11-strips/p07.pddl --search --if-unit-cost "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=true), let(hff, ff(), lazy_greedy([hff,hlm],preferred=[hff,hlm]) ))" --if-non-unit-cost "let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=true), let(hff1, ff(transform=adapt_costs(one)), lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1], cost_type=one,reopen_closed=false) ))"> $FOLDER/sokoban.txt 2>&1
grep Expanded "$FOLDER/sokoban.txt"
grep -i "Plan valid" "$FOLDER/sokoban.txt"
echo

echo "testing parcprinter"
./fast-downward.py --validate benchmarks-test/parcprinter-sat11-strips/p12-domain.pddl benchmarks-test/parcprinter-sat11-strips/p12.pddl --search --if-unit-cost "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=true), let(hff, ff(), lazy_greedy([hff,hlm],preferred=[hff,hlm]) ))" --if-non-unit-cost "let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=true), let(hff1, ff(transform=adapt_costs(one)), lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1], cost_type=one,reopen_closed=false) ))"> $FOLDER/tidybot.txt 2>&1
grep Expanded "$FOLDER/parcprinter.txt"
grep -i "Plan valid" "$FOLDER/parcprinter.txt"
echo

# echo "testing scanalyzer"
# ./fast-downward.py --validate benchmarks-test/scanalyzer-sat11-strips/domain.pddl benchmarks-test/scanalyzer-sat11-strips/p20.pddl $EVALS --search "eager($1, cost_type=$COST)"> $FOLDER/tidybot.txt 2>&1
# grep Expanded "$FOLDER/scanalyzer.txt"
# grep -i "Plan valid" "$FOLDER/scanalyzer.txt"
# echo

echo "testing floortile"
./fast-downward.py --validate benchmarks-test/floortile-sat11-strips/domain.pddl benchmarks-test/floortile-sat11-strips/seq-p01-002.pddl --search --if-unit-cost "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=true), let(hff, ff(), lazy_greedy([hff,hlm],preferred=[hff,hlm]) ))" --if-non-unit-cost "let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=true), let(hff1, ff(transform=adapt_costs(one)), lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1], cost_type=one,reopen_closed=false) ))"> $FOLDER/floortile.txt 2>&1
grep Expanded "$FOLDER/floortile.txt"
grep -i "Plan valid" "$FOLDER/floortile.txt"
echo

# echo "testing ged"
# ./fast-downward.py --validate benchmarks-test/ged-sat14-strips/domain.pddl benchmarks-test/ged-sat14-strips/d-10-3.pddl $EVALS --search "eager($1, cost_type=$COST)"> $FOLDER/ged.txt 2>&1
# grep Expanded "$FOLDER/ged.txt"
# grep -i "Plan valid" "$FOLDER/ged.txt"
# echo