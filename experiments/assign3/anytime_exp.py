#! /usr/bin/env python3

import os
import shutil

import common_setup
from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment
from lab.reports import Attribute, arithmetic_mean

def _anytime_props_processor(props: dict, **kwargs):

    def add_task_data(k, algo, domain):
        prop = props[k]
        
        # average time between solutions -- include proof of optimality time step?
        times_between_sols: list = prop["time:steps"]
        avg_time = sum(times_between_sols)/len(times_between_sols)
        prop["time:steps:avg"] = avg_time

        # first change index if more than one solution found
        incumbent_costs = prop["cost:all"]
        if len(incumbent_costs)>1:
            first_change: list = prop["change_indices"][0]
            prop["change_indices:first"] = first_change / incumbent_costs[0]

        # todo: quality per time
        
    # def update_domain_summary(k, algo, domain):
    #     prop = props[k]
    #     prop["update_domain_summary"] = domain

    # def update_algo_summary(k, algo, domain):
    #     prop = props[k]
    #     prop["update_algo_summary"] = algo
    
    for k, prop in props.items():
        algo = prop["algorithm"]
        domain = prop["domain"]

        add_task_data(k, algo, domain)
        # update_domain_summary(k, algo, domain)
        # update_algo_summary(k, algo, domain)


REPO = common_setup.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
SEARCH_REVS = ["dawson-masters"]
BUILD_OPTIONS = []
DRIVER_OPTIONS = ["--overall-time-limit", "10m"]
ENVIRONMENT = LocalEnvironment(processes=None)
SUITE = common_setup.get_ipcs_sat_domains()

CONFIGS = [
    IssueConfig("AWA*", ["--evaluator", "h=lmcut()", '--search',
        """eager_anytime(single(sum([weight(h, 2, verbosity=normal), g()])), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
    IssueConfig("e-AWA*", ["--evaluator", "h=lmcut()", "--search",
        """eager_anytime(epsilon_greedy(
        sum([weight(h, 2, verbosity=normal), g()]), epsilon=0.5, random_seed=1234), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
    IssueConfig("Type-AWA*", ["--evaluator", "h=lmcut()", "--search",
        """eager_anytime(alt(
        [single(weight(h, 2, verbosity=normal)), type_based([h, g()], random_seed=1234)]), 
        reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
    # IssueConfig("RWA*", ["--evaluator", "h=lmcut()", "--search",
    #     """iterated([
    #         eager_wastar([h],w=5),
    #         eager_wastar([h],w=4),
    #         eager_wastar([h],w=3),
    #         eager_wastar([h],w=2),
    #         eager_wastar([h],w=1)
    #     ],continue_on_fail=true)"""], driver_options=DRIVER_OPTIONS),
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
exp.add_absolute_report_step(attributes=[
    Attribute("time:steps:avg", min_wins=True, function=arithmetic_mean), 
    Attribute("change_indices:avg", min_wins=True, function=arithmetic_mean),
    Attribute("cost", min_wins=True, function=arithmetic_mean),
    "coverage", "expansions", "generated"])


# exp.add_comparison_table_step(attributes=["expansions"])
# exp.add_scatter_plot_step(relative=True, attributes=["expansions"])

exp.run_steps()

