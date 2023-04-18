#! /usr/bin/env python3

import os
from pprint import pprint
import shutil
from typing import List
import matplotlib.pyplot as plt

import common_setup
from common_setup import IssueConfig, IssueExperiment
from lab.environments import LocalEnvironment
from lab.reports import Attribute, arithmetic_mean, geometric_mean, finite_sum

from dataclasses import dataclass


@dataclass
class IncumbentSolution:
    """A class keeping track of an incumbent solution."""
    problem: str
    domain: str
    cost: float = 0
    time: int = 0

    def __lt__(self, other):
        return self.time < other.time
    


def _anytime_props_processor(props: dict, **kwargs):

    problem_best_sol_cost = dict()
    algo_incumbents = dict()
    domains = []

    def add_task_data(k, algo, domain):

        prop = props[k]
        incumbent_costs = prop["cost:all"]
        time_steps = prop["time:steps"]
        time_totals = prop["time:total"]
        problem = prop["problem"]
        domains.append(domain)

        # average time between solutions
        # this includes the time it takes to prove the optimal solution is actually optimal
        if len(time_steps)>=1:
            avg_time = sum(time_steps)/len(time_steps)
            prop["time:steps:avg"] = avg_time

        # first change index if more than one solution found
        if len(prop["change_indices"])>0:
            first_change: list = prop["change_indices"][0]
            prop["change_index:first"] = first_change / incumbent_costs[0]

        # update problem best solution
        if len(incumbent_costs)>=1:
            if problem in problem_best_sol_cost:
                this_sol = incumbent_costs[-1]
                best_sol = problem_best_sol_cost[problem]
                if this_sol < best_sol:
                    problem_best_sol_cost[problem] = this_sol
            else:
                problem_best_sol_cost[problem] = incumbent_costs[-1]
        
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
        algo_incumbents[algo].extend([IncumbentSolution(problem, domain, cost, time) for cost, time in zip(incumbent_costs, time_totals)])


    def normalize_and_sort_costs():
        incumbents: List[IncumbentSolution]
        for _, incumbents in algo_incumbents.items():
            incumbents.sort()
        
        for _, incumbents in algo_incumbents.items():
            for incumbent in incumbents:
                incumbent.cost = problem_best_sol_cost[incumbent.problem]/incumbent.cost

    def calculate_quality_v_time_points():

        def calculate_for_algo_block(algo_block: dict, filter_domain=None) -> dict:
            quality_v_time = dict()
            incumbents: List[IncumbentSolution]
            for algo, incumbents in algo_block.items():
                if algo not in quality_v_time:
                    quality_v_time[algo] = dict()
                    quality_v_time[algo]["quality"] = []
                    quality_v_time[algo]["time"] = []

                problem_curr_best_sol = dict.fromkeys(problem_best_sol_cost.keys(),0)
                i = 0
                while i < len(incumbents):

                    if filter_domain is not None and filter_domain != incumbents[i].domain:
                        i+=1
                        continue

                    time = incumbents[i].time

                    # update problem "curr"
                    j = i
                    while j < len(incumbents) and incumbents[j].time == time: j+=1
                    for incumbent in incumbents[i:j]:
                        problem_curr_best_sol[incumbent.problem] = incumbent.cost

                    quality_v_time[algo]["quality"].append(
                        sum([quality for _,quality in problem_curr_best_sol.items()]) / len(problem_curr_best_sol)
                    )
                    quality_v_time[algo]["time"].append(time)
                    i=j 

            return quality_v_time

        normalize_and_sort_costs() 
        quality_v_time_domain = dict()       
        quality_v_time_all = calculate_for_algo_block(algo_incumbents, filter_domain=None)
        for domain in domains:
            quality_v_time_domain[domain] = calculate_for_algo_block(algo_incumbents, filter_domain=domain)              

        return quality_v_time_all, quality_v_time_domain

    def save_quality_v_time_scatter_plots(quality_v_time_all: dict, quality_v_time_domain: dict):

        # all domains
        for algo_long, algo_data in quality_v_time_all.items():
            algo = algo_long[15:]
            plt.plot(algo_data["time"], algo_data["quality"], '-o', label=algo)
        
        plt.legend(loc='best')
        plt.title("Solution Quality vs. Time")
        plt.xlabel("Time (milliseconds)")
        plt.ylabel("Solution Quality (C/C*)")
        plt.xscale('log')
        plt.savefig(f'quality_v_time.png', dpi=400)

        plt.cla()
        plt.clf()

        # per domain
        # all domains
        for domain, algo_block in quality_v_time_domain.items():
            for algo_long, algo_data in algo_block.items():
                algo = algo_long[15:]
                plt.plot(algo_data["time"], algo_data["quality"], '-o', label=algo)
            
            plt.legend(loc='best')
            plt.title("Solution Quality vs. Time")
            plt.xlabel("Time (milliseconds)")
            plt.ylabel(f"{domain} - Solution Quality (C/C*)")
            plt.xscale('log')
            plt.savefig(f'{domain} - quality_v_time.png', dpi=400)
            
            plt.cla()
            plt.clf()

    for k, prop in props.items():
        algo = prop["algorithm"]
        domain = prop["domain"]
        add_task_data(k, algo, domain)
    
    all_scatter, domain_scatter = calculate_quality_v_time_points()
    save_quality_v_time_scatter_plots(all_scatter, domain_scatter)
    # pprint(quality_v_time)
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
    IssueConfig("RWA*", ["--evaluator", "h=lmcut()", "--search",
        """iterated([
            lazy_wastar([h],w=5),
            lazy_wastar([h],w=4),
            lazy_wastar([h],w=3),
            lazy_wastar([h],w=2),
            astar(h)
        ],continue_on_fail=true)"""], driver_options=DRIVER_OPTIONS),
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
    Attribute("time:steps:optimal:avg", min_wins=True, function=arithmetic_mean),
    Attribute("change_index:first", min_wins=True, function=arithmetic_mean),
    Attribute("cost", min_wins=True, function=arithmetic_mean),
    "coverage", 
    Attribute("coverage:optimal", min_wins=True, absolute=True, function=finite_sum), 
    Attribute("expansions", min_wins=True, function=geometric_mean),
    Attribute("expansions:optimal", min_wins=True, function=geometric_mean),
    Attribute("planner_time:optimal", min_wins=True, function=geometric_mean)])


# exp.add_comparison_table_step(attributes=["expansions"])
# exp.add_scatter_plot_step(relative=True, attributes=["expansions"])

exp.run_steps()

