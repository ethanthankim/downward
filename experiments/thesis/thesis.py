#! /usr/bin/env python3

import os
import shutil

import project
import common_setup

from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment


REPO = common_setup.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
SEARCH_REVS = ["dawson-masters"]
BUILD_OPTIONS = []
DRIVER_OPTIONS = ["--overall-time-limit", "5m"]
ENVIRONMENT = LocalEnvironment(processes=None)
SUITE = common_setup.get_ipcs_sat_domains()

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
    # ('ff', ['--search', 'eager_greedy([ff()])']),
    # ('ff-def', ['--search', 'lazy_greedy([ff()])']),
    # ('ff-pref', ['--search', 'eager_greedy([ff()], preferred=[ff()])']),
    # ('ff-pref-boost', ['--search', 'eager_greedy([ff()], preferred=[ff()], boost=1000)']),
    # ('ff-def-boost', ['--search', 'lazy_greedy([ff()], boost=1000)']),
    # ('lama-2011', _get_lama(pref="true"))
    IssueConfig('lwm', ["--evaluator", "h=lmcut()", '--search', 'eager(lwm_based_type(h))'], driver_options=DRIVER_OPTIONS)
]

BUILD_OPTIONS = []
DRIVER_OPTIONS = ["--overall-time-limit", "5m"]
REVS = [
    ("main", "main"),
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
exp.add_comparison_table_step(attributes=["expansions"])

exp.run_steps()
