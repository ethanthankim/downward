#! /usr/bin/env python3

from argparse import Namespace
import os

import project
import common_setup
from lab.experiment import ARGPARSER

from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment

def parse_args():
    ARGPARSER.add_argument(
        "--test",
        choices=["yes", "no", "auto"],
        default="auto",
        dest="test_run",
        help="test experiment locally on a small suite if --test=yes or "
             "--test=auto and we are not on a cluster")
    
    ARGPARSER.add_argument(
        "--no-search",
        action='store_true',
        default=False,
        dest="no_search",
        help="if set only fetching and parsing steps will be run.")
    
    ARGPARSER.add_argument(
        "--benchmark-dir",
        default=os.environ["DOWNWARD_BENCHMARKS"],
        dest="benchmark_dir",
        help="set the benchmark directory. Defaults to DOWNWARD_BENCHMARKS environment variable."
    )

    ARGPARSER.add_argument(
        "--overall-time-limit", 
        default="5m",
        dest="time_limit",
        help="the overall time limit for each search algorithm."
    )

    
    return ARGPARSER.parse_args()


def main():
    ARGS = parse_args()

    REPO = common_setup.get_repo_base()
    BENCHMARKS_DIR = ARGS.benchmark_dir
    REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
    SEARCH_REVS = ["dawson-masters"]
    BUILD_OPTIONS = []
    DRIVER_OPTIONS = ["--overall-time-limit", ARGS.time_limit]
    ENVIRONMENT = LocalEnvironment(processes=None)
    SUITE = common_setup.get_suite(ARGS.benchmark_dir)

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
        IssueConfig('epsilon-0.2', ["--evaluator", "h=lmcut()", '--search', 'eager(epsilon_greedy(h), epsilon=0.2)'], driver_options=DRIVER_OPTIONS),
        IssueConfig('type', ["--evaluator", "h=lmcut()", '--search', 'eager(alt([type_based([h, g()], random_seed=-1), single(h)]))'], driver_options=DRIVER_OPTIONS),
        IssueConfig('gbfs', ["--evaluator", "h=lmcut()", '--search', 'eager(single(h))'], driver_options=DRIVER_OPTIONS),
        
    ]
    ATTRIBUTES = [
        "error",
        "total_time",
        "coverage",
        "expansions",
        "memory",
        project.EVALUATIONS_PER_TIME,
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
    exp.add_parser(project.DIR / "parser.py")
    exp.add_parser(exp.PLANNER_PARSER)

    exp.add_step("build", exp.build)
    exp.add_step("start", exp.start_runs)
    exp.add_fetcher(name="fetch")

    project.add_absolute_report(
        exp, attributes=ATTRIBUTES, filter=[project.add_evaluations_per_time]
    )
    exp.add_comparison_table_step(attributes=["expansions", "total_time"])

    exp.run_steps()



if __name__ == '__main__':
    main()

