#! /usr/bin/env python3

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
    ENVIRONMENT = LocalEnvironment(processes=None)
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

    CONFIGS = [
        IssueConfig('epsilon-0.2', ["--evaluator", "h=ff()", '--search', 'eager(epsilon_greedy(h, epsilon=0.2))'], driver_options=DRIVER_OPTIONS),
        IssueConfig('type', ["--evaluator", "h=ff()", '--search', 'eager(alt([type_based([h, g()], random_seed=-1), single(h)]))'], driver_options=DRIVER_OPTIONS),
        # IssueConfig('gbfs', ["--evaluator", "h=ff()", '--search', 'eager(single(h))'], driver_options=DRIVER_OPTIONS),
        IssueConfig('lwm-intra', ["--evaluator", "h=ff()", '--search', 'eager( alt( [lwm_intra_explore_type(h), single(h)] ) )'], driver_options=DRIVER_OPTIONS),
        IssueConfig('bts', ["--evaluator", "h=ff()", '--search', 'eager(alt([bts_type(h), single(h)]))'], driver_options=DRIVER_OPTIONS),
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


