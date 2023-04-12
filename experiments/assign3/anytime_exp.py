#! /usr/bin/env python3

import os
import shutil

import common_setup
from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment

def _anytime_props_processor(props: dict, **kwargs):

    def add_run_data(k, algo, domain):
        prop = props[k]
        prop["add_run_data"] = k

    def update_domain_summary(k, algo, domain):
        prop = props[k]
        prop["update_domain_summary"] = domain

    def update_algo_summary(k, algo, domain):
        prop = props[k]
        prop["update_algo_summary"] = algo
    
    for k, prop in props.items():
        algo = prop["algorithm"]
        domain = prop["domain"]

        add_run_data(k, algo, domain)
        update_domain_summary(k, algo, domain)
        update_algo_summary(k, algo, domain)


REPO = common_setup.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
SEARCH_REVS = ["dawson-masters"]
BUILD_OPTIONS = [
    "DPLUGIN_CEGAR_ENABLED=False",
    "DPLUGIN_PDBS_ENABLED=False",
    # "-DPLUGIN_OPERATOR_COUNTING_ENABLED=False",
    # "-DPLUGIN_LANDMARKS_ENABLED=False",
    # "-DPLUGIN_MAS_HEURISTIC_ENABLED=False",
    # "-DPLUGIN_POTENTIALS_ENABLED=False"
]
DRIVER_OPTIONS = ["--overall-time-limit", "10m"]
ENVIRONMENT = LocalEnvironment(processes=None)
SUITE = common_setup.get_ipcs_sat_domains()

CONFIGS = [
    IssueConfig("AWA*", ["--evaluator", "h=lmcut()", '--search',
        """eager_anytime(single(sum([weight(h, 2, verbosity=normal), g()])), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS, build_options=BUILD_OPTIONS),
    IssueConfig("e-AWA*", ["--evaluator", "h=lmcut()", "--search",
        """eager_anytime(epsilon_greedy(
        sum([weight(h, 2, verbosity=normal), g()]), epsilon=0.5, random_seed=1234), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS, build_options=BUILD_OPTIONS),
    IssueConfig("Type-AWA*", ["--evaluator", "h=lmcut()", "--search",
        """eager_anytime(alt(
        [single(weight(h, 2, verbosity=normal)), type_based([h, g()], random_seed=1234)]), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS, build_options=BUILD_OPTIONS),
    IssueConfig("RWA*", ["--evaluator", "h=lmcut()", "--search",
        """iterated([
            eager_wastar([h],w=5),
            eager_wastar([h],w=4),
            eager_wastar([h],w=3),
            eager_wastar([h],w=2),
            eager_wastar([h],w=1)
        ],continue_on_fail=true)"""], driver_options=DRIVER_OPTIONS, build_options=BUILD_OPTIONS),
]

exp = IssueExperiment(
    revisions=SEARCH_REVS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)


exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
# exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(common_setup.DIR / "anytime_parser.py")
# exp.add_parser(common_setup.DIR / "common_parser.py")
exp.add_parser(exp.PLANNER_PARSER)

if not common_setup.no_search():
    exp.add_step("build", exp.build)
    exp.add_step("start", exp.start_runs)

exp.add_fetcher(name="fetch")
exp.add_properties_processing_step({"anytime-experiment": _anytime_props_processor})
# exp.add_comparison_table_step(attributes=["expansions"])
# exp.add_scatter_plot_step(relative=True, attributes=["expansions"])

exp.run_steps()

