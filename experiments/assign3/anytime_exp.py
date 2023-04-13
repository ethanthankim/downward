#! /usr/bin/env python3

import os
from pprint import pprint
import shutil
from typing import List

import common_setup
from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment
from lab.reports import Attribute, arithmetic_mean

from dataclasses import dataclass


@dataclass
class IncumbentSolution:
    """A class keeping track of an incumbent solution."""
    problem: str
    cost: float = 0
    time: int = 0

    def __lt__(self, other):
        return self.time < other.time
    


def _anytime_props_processor(props: dict, **kwargs):

    problem_summaries = dict()
    algo_incumbents = dict()
    quality_v_time = dict()

    def add_task_data(k, algo, domain):

        prop = props[k]
        incumbent_costs = prop["cost:all"]
        time_steps = prop["time:steps"]
        time_totals = prop["time:total"]
        problem = prop["problem"]

        # average time between solutions
        # this includes the time it takes to prove the optimal solution is actually optimal
        if len(time_steps)>=1:
            avg_time = sum(time_steps)/len(time_steps)
            prop["time:steps:avg"] = avg_time

        # first change index if more than one solution found
        if len(incumbent_costs)>1:
            first_change: list = prop["change_indices"][0]
            prop["change_indices:first"] = first_change / incumbent_costs[0]

        # update problem best solution
        if len(incumbent_costs)>=1:
            if problem in problem_summaries:
                this_sol = incumbent_costs[-1]
                best_sol = problem_summaries[problem]["best_solution"]
                if this_sol < best_sol:
                    problem_summaries[problem]["best_solution"] = this_sol
            else:
                problem_sum = dict()
                problem_sum["best_solution"] = incumbent_costs[-1]
                problem_sum["curr"] = 0
                problem_summaries[problem] = problem_sum
        
        # being in here means the optimal solution was proved optimal
        if "optimal:found" in prop and prop["optimal:found"] == "found":
            prop["coverage:optimal"] = 1
            prop["expansions:optimal"] = prop["expansions"]
            prop["planner_time:optimal"] = prop["planner_time"]
            if len(time_steps)>=1:
                avg_time = sum(time_steps)/len(time_steps)
                prop["time:steps:optimal:avg"] = avg_time
        else:
            prop["coverage:optimal"] = 0

        # algo_summaries
        if algo not in algo_incumbents:
            algo_incumbents[algo] = []
        algo_incumbents[algo].extend([IncumbentSolution(problem, cost, time) for cost, time in zip(incumbent_costs, time_totals)])

    def normalize_and_sort_costs():
        incumbents: List[IncumbentSolution]
        for _, incumbents in algo_incumbents.items():
            incumbents.sort()
        
        for _, incumbents in algo_incumbents.items():
            for incumbent in incumbents:
                incumbent.cost = problem_summaries[incumbent.problem]["best_solution"]/incumbent.cost

    def build_quality_v_time_points():
        normalize_and_sort_costs()
        
        incumbents: List[IncumbentSolution]
        for algo, incumbents in algo_incumbents.items():
            
            if algo not in quality_v_time:
                quality_v_time[algo] = dict()
                quality_v_time[algo]["quality"] = []
                quality_v_time[algo]["time"] = []
            
            print(algo)
            pprint(incumbents)
            i = 0
            while i < len(incumbents):
                time = incumbents[i].time

                # update problem "curr"
                j = i
                while j < len(incumbents) and incumbents[j].time == time: j+=1
                for incumbent in incumbents[i:j]:
                    problem_summaries[incumbent.problem]["curr"] = incumbent.cost

                quality_v_time[algo]["quality"].append(
                    sum([problem["curr"] for _,problem in problem_summaries.items()]) / len(problem_summaries)
                )
                quality_v_time[algo]["time"].append(time)
                i=j

            for _,problem in problem_summaries.items():
                problem["curr"] = 0

                
            
    
    for k, prop in props.items():
        algo = prop["algorithm"]
        domain = prop["domain"]

        add_task_data(k, algo, domain)
    
    build_quality_v_time_points()
    pprint(quality_v_time)
    # pprint(problem_summaries)


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

