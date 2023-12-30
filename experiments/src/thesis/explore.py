#! /usr/bin/env python3

from datetime import datetime
import multiprocessing
import experiment_setup.common_setup as setup
from experiment_setup.common_setup import IssueConfig, IssueExperiment


import os
from lab.environments import LocalEnvironment 

def main():

    REPO = setup.get_repo_base()
    BENCHMARKS_DIR = setup.get_benchmark_dir()
    REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
    SEARCH_REVS = ["dawson-masters"]
    BUILD_OPTIONS = []
    DRIVER_OPTIONS = ["--overall-time-limit", setup.get_time_limit()]
    ENVIRONMENT = LocalEnvironment(processes=multiprocessing.cpu_count()-1)
    SUITE = setup.get_suite(BENCHMARKS_DIR)

    def _get_lama(pref):
        return [
            "--search",
            "--if-unit-cost",
            f"let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref={pref}),"
            "let(hff, ff(),"
            """iterated([
                lazy_greedy([hff,hlm],preferred=[hff,hlm]),
                lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),
                lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),
                lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),
                lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)
            ],repeat_last=true,continue_on_fail=true)))""",
            "--if-non-unit-cost",
            f"let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref={pref}),"
            "let(hff1, ff(transform=adapt_costs(one)),"
            f"let(hlm2, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(plusone),pref={pref}),"
            "let(hff2, ff(transform=adapt_costs(plusone)),"
            """iterated([
                lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
                    cost_type=one,reopen_closed=false),
                lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
                    reopen_closed=false),
                lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
                lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
                lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
                lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)
            ],repeat_last=true,continue_on_fail=true)))))""",
            # Append --always to be on the safe side if we want to append
            # additional options later.
            "--always"]

    normal_ff = "ff()"
    unit_ff = "ff(transform=adapt_costs(cost_type=one))"
    unit_cost = 'cost_type=one'
    real_cost = 'cost_type=normal'

    FF_RANDOM_SEED = int(datetime.now().timestamp())
    CONFIGS = [
               
        IssueConfig('HIB', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt( [ single(h), hib_partition(h, biased_level(tau=1, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED})) ] ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        IssueConfig('LWMB', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt( [ single(h), lwmb_partition(h, biased_level(tau=1, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED})) ] ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),

    ]

    ATTRIBUTES = [
        "error",
        "total_time",
        "coverage",
        "expansions",
        "memory",
        "cost"
    ]

    exp = IssueExperiment(
        revisions=SEARCH_REVS,
        configs=CONFIGS,
        environment=ENVIRONMENT,
    )
    exp.add_suite(BENCHMARKS_DIR, SUITE)

    exp.add_parser(exp.EXITCODE_PARSER)
    exp.add_parser(exp.TRANSLATOR_PARSER)
    exp.add_parser(exp.SINGLE_SEARCH_PARSER)
    exp.add_parser(setup.DIR / "common_parser.py")
    exp.add_parser(exp.PLANNER_PARSER)

    if not setup.no_search():
        exp.add_step("build", exp.build)
        exp.add_step("start", exp.start_runs)
    exp.add_fetcher(name="fetch")

    exp.add_absolute_report_step(attributes=ATTRIBUTES)
    # exp.add_comparison_table_step(attributes=["expansions", "total_time"])

    exp.run_steps()


if __name__ == "__main__":
    main()


