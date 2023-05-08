#! /usr/bin/env python3

import os
import shutil

import project
import common_setup


REPO = project.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
SCP_LOGIN = "myname@myserver.com"
REMOTE_REPOS_DIR = "/infai/seipp/projects"
# If REVISION_CACHE is None, the default ./data/revision-cache is used.
REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
SUITE = common_setup.get_ipcs_sat_domains()

ENV = project.LocalEnvironment(processes=None)

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
    ('lwm', ["--evaluator", "h=lmcut()", '--search', 'eager(lwm_based_type(h))'])
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

exp = project.FastDownwardExperiment(environment=ENV, revision_cache=REVISION_CACHE)
for config_nick, config in CONFIGS:
    for rev, rev_nick in REVS:
        algo_name = f"{rev_nick}:{config_nick}" if rev_nick else config_nick
        exp.add_algorithm(
            algo_name,
            REPO,
            rev,
            config,
            build_options=BUILD_OPTIONS,
            driver_options=DRIVER_OPTIONS,
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

attributes = ["expansions"]
pairs = [
    ("20.06:01-cg", "20.06:02-ff"),
]
suffix = "-rel" if project.RELATIVE else ""
for algo1, algo2 in pairs:
    for attr in attributes:
        exp.add_report(
            project.ScatterPlotReport(
                relative=project.RELATIVE,
                get_category=None if project.TEX else lambda run1, run2: run1["domain"],
                attributes=[attr],
                filter_algorithm=[algo1, algo2],
                filter=[project.add_evaluations_per_time],
                format="tex" if project.TEX else "png",
            ),
            name=f"{exp.name}-{algo1}-vs-{algo2}-{attr}{suffix}",
        )

exp.run_steps()
