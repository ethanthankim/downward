#! /usr/bin/env python3

import os
import shutil

import project


REPO = project.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
SCP_LOGIN = "myname@myserver.com"
REMOTE_REPOS_DIR = "/infai/seipp/projects"
# If REVISION_CACHE is None, the default ./data/revision-cache is used.
REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
# exit()
SUITE = project._get_suite("suite")
# SUITE = ["depot:p01.pddl", "grid:prob01.pddl", "gripper:prob01.pddl"]

ENV = project.LocalEnvironment(processes=None)

AWA_CONFIG = [
    "--evaluator",
    "h=lmcut()",
    '--search',
    "eager_anytime(single(sum([weight(h, 2, verbosity=normal), g()])), reopen_closed=true, f_eval=sum([h, g()]))"
]

# AWA_WS_CONFIG = [
#     "--evaluator",
#     "h=ff()",
#     "--search",
#     """iterated([
#             lazy_greedy([h]),
#             lazy_wastar([h],w=5),
#             lazy_wastar([h],w=3),
#             lazy_wastar([h],w=2),
#             lazy_wastar([h],w=1)
#         ],continue_on_fail=true)"""
# ]

EAWA_CONFIG = [
    "--evaluator",
    "h=lmcut()",
    "--search",
    """iterated([
            lazy_greedy([h]),
            lazy(epsilon_greedy(weight([h], 2, verbosity=normal), pref_only=false, epsilon=0.5, random_seed=-1))
        ],repeat_last=true,continue_on_fail=true)""" 
]

TYPE_AWA_CONFIG = [
    "--evaluator",
    "h=lmcut()",
    "--search",
    """iterated([
            lazy_greedy([h]),
            lazy(alt([single(weight([h], 2, verbosity=normal)), type_based([h, g()], random_seed=-1)])
        ],repeat_last=true,continue_on_fail=true)""" 
]

# EAWA_WS_CONFIG = [
#     "--evaluator",
#     "h=ff()",
#     "--search",
#     """iterated([
#             lazy_greedy([h]),
#             lazy(epsilon_greedy(weight(h, 5, verbosity=normal), pref_only=false, epsilon=0.5, random_seed=-1)),
#             lazy(epsilon_greedy(weight(h, 3, verbosity=normal), pref_only=false, epsilon=0.5, random_seed=-1)),
#             lazy(epsilon_greedy(weight(h, 2, verbosity=normal), pref_only=false, epsilon=0.5, random_seed=-1)),
#             lazy(epsilon_greedy(weight(h, 1, verbosity=normal), pref_only=false, epsilon=0.5, random_seed=-1)),
#             lazy_wastar(h,w=1)
#         ],continue_on_fail=true)""" 
# ]

# TYPE_AWA_WS_CONFIG = [
#     "--evaluator",
#     "h=ff()",
#     "--search",
#     """iterated([
#             lazy_greedy([h]),
#             lazy(alt([single(weight(h, 5, verbosity=normal)), type_based([h, g()], random_seed=-1)]),
#             lazy(alt([single(weight(h, 3, verbosity=normal)), type_based([h, g()], random_seed=-1)]),
#             lazy(alt([single(weight(h, 2, verbosity=normal)), type_based([h, g()], random_seed=-1)]),
#             lazy(alt([single(weight(h, 1, verbosity=normal)), type_based([h, g()], random_seed=-1)]),
#             lazy_wastar(h,w=1)
#         ],continue_on_fail=true)""" 
# ]

RWA_CONFIG = [

]
    
CONFIGS = [
    ('AWA', AWA_CONFIG),
    # ('EAWA', EAWA_CONFIG),
    # ('TYPE_AWA', TYPE_AWA_CONFIG),

    # ('AWA-WS', AWA_WS_CONFIG),
    # ('EAWA-WS', EAWA_WS_CONFIG),
    # ('TYPE_AWA_WS', TYPE_AWA_WS_CONFIG)
]

BUILD_OPTIONS = []
DRIVER_OPTIONS = ["--overall-time-limit", "10m"]
REVS = [
    ("dawson-masters", "assign3"),
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
# exp = project.FastDownwardExperiment(environment=ENV)
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
# exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
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
    ("AWA", "AWA_WS"),
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
