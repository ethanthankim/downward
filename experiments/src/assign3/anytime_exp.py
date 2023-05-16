#! /usr/bin/env python3

import os
from pprint import pprint
import shutil
from typing import Dict, List
import matplotlib.pyplot as plt
import re
import json

from experiment_setup import common_setup as setup
from experiment_setup.common_setup import IssueConfig, IssueExperiment
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

    eval_dir = kwargs['eval']
    problem_best_sol_cost = dict()
    algo_incumbents = dict()
    domains = []
    algo_change_indices: Dict[str, List[List[float]]] = dict()

    def _problem_key(domain, problem):
        return f'{domain}-{problem}'

    def add_task_data(k, algo, domain):

        prop = props[k]
        domains.append(domain)

        incumbent_costs = prop["cost:all"]
        incumbent_steps = prop["steps:all"]
        time_steps = prop["time:steps"]
        time_totals = prop["time:total"]
        problem = prop["problem"]
        problem_key = _problem_key(domain, problem)

        # average time between solutions
        # this includes the time it takes to prove the optimal solution is actually optimal
        if len(time_steps)>=1:
            avg_time = sum(time_steps)/len(time_steps)
            prop["time:steps:avg"] = avg_time

        # normalize change index
        prop["change_indices"] = [ci / incumbent_steps[i] for i,ci in enumerate(prop["change_indices"])]
        
        # all change indices per incumbent solution index
        if algo not in algo_change_indices:
            algo_change_indices[algo] = []
        for i, ci in enumerate(prop["change_indices"]):
            if len(algo_change_indices[algo]) <= i:
                algo_change_indices[algo].append([])
            algo_change_indices[algo][i].append(ci)

        # first change index if more than one solution found
        if len(prop["change_indices"])>0:
            prop["change_index:first"] = prop["change_indices"][0]

        # second change index
        if len(prop["change_indices"])>1:
            prop["change_index:second"] = prop["change_indices"][1]

        # third change index
        if len(prop["change_indices"])>2:
            prop["change_index:third"] = prop["change_indices"][2]

        # update problem best solution
        if "cost" in prop:
            this_sol = prop["cost"]
            if problem_key in problem_best_sol_cost:
                best_sol = problem_best_sol_cost[problem_key]
                if this_sol < best_sol:
                    problem_best_sol_cost[problem_key] = this_sol
            else:
                problem_best_sol_cost[problem_key] = this_sol
        
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
            for incumbent in incumbents:
                incumbent.cost = problem_best_sol_cost[_problem_key(incumbent.domain, incumbent.problem)]/incumbent.cost

        for _, incumbents in algo_incumbents.items():
            incumbents.sort()

    def calculate_ci_v_si() -> dict:
        ci_v_si: Dict[str, Dict[str, List[float]]] = dict()
        for algo, cis in algo_change_indices.items():
            if algo not in ci_v_si:
                ci_v_si[algo] = dict()
                ci_v_si[algo]['incumbent'] = []
                ci_v_si[algo]['c.i.'] = []

            for i,l in enumerate(cis):
                ci_v_si[algo]['incumbent'].append(i)
                ci_v_si[algo]['c.i.'].append( sum(l) / len(l) )

        return ci_v_si

    def save_ci_v_si_scatter_plots(ci_v_si: dict):
        
        for algo_long, algo_data in ci_v_si.items():
            algo = algo_long[15:]
            plt.plot(algo_data["incumbent"], algo_data["c.i."], '-o', label=algo)
            
            plt.legend(loc='best')
            plt.title("Average Change Index per Incumbent Solution")
            plt.xlabel("Incumbent Solution Index")
            plt.ylabel("Average Change Index")
            plt.savefig(os.path.join(eval_dir, "average_ci_v_si"), dpi=400)   

    def calculate_quality_v_time_points():

        def calculate_for_algo_block(algo_block: dict, filter_domain=None) -> dict:
            quality_v_time = dict()
            incumbents_all: List[IncumbentSolution]
            for algo, incumbents_all in algo_block.items():
                incumbents = [incumbent for incumbent in incumbents_all if filter_domain == incumbent.domain or filter_domain is None]
                problem_curr_best_sol = dict.fromkeys([_problem_key(incumbent.domain, incumbent.problem) for incumbent in incumbents], 0)

                if algo not in quality_v_time:
                    quality_v_time[algo] = dict()
                    quality_v_time[algo]["quality"] = []
                    quality_v_time[algo]["time"] = []

                i = 0
                while i < len(incumbents):

                    time = incumbents[i].time

                    # update problem "curr"
                    j = i
                    while j < len(incumbents) and incumbents[j].time == time: j+=1
                    for incumbent in incumbents[i:j]:
                        problem_curr_best_sol[_problem_key(incumbent.domain, incumbent.problem)] = incumbent.cost

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
        SYMBOLS = r'[\.-_\(\)]'
        def title_to_filename(title: str) -> str:
            filename = re.sub(SYMBOLS, '', title.lower())
            return filename.replace(' ', '_')

        def save_algo_block_scatter(algo_block: dict, x_name: str, y_name: str, title: str, scale='linear'):
            for algo_long, algo_data in algo_block.items():
                algo = algo_long[15:]
                plt.plot(algo_data["time"], algo_data["quality"], '-o', label=algo)
            
            plt.legend(loc='best')
            plt.title(title)
            plt.xlabel(x_name)
            plt.ylabel(y_name)
            plt.xscale('log')
            plt.savefig(os.path.join(eval_dir, title_to_filename(title)), dpi=400)


        # all domains
        save_algo_block_scatter(quality_v_time_all, 
                                "Time (milliseconds)", 
                                "Solution Quality (C*/C)",
                                "Solution Quality vs. Time",
                                'log')
        plt.cla()
        plt.clf()

        # per domain
        for domain, algo_block in quality_v_time_domain.items():
            save_algo_block_scatter(algo_block, 
                                "Time (milliseconds)", 
                                "Solution Quality (C/C*)",
                                f"{domain} - Solution Quality vs. Time",
                                'log')
            plt.cla()
            plt.clf()
            

    for k, prop in props.items():
        algo = prop["algorithm"]
        domain = prop["domain"]

        if "unexplained_errors" in prop:
            continue

        add_task_data(k, algo, domain)
    
    all_scatter, domain_scatter = calculate_quality_v_time_points()
    save_quality_v_time_scatter_plots(all_scatter, domain_scatter)
    avg_cis = calculate_ci_v_si()
    save_ci_v_si_scatter_plots(avg_cis)

    with open(os.path.join(eval_dir, 'quality_v_time_all.json'), 'w', encoding='utf-8') as f: 
        json.dump(all_scatter, f, ensure_ascii=False, indent=4)
    with open(os.path.join(eval_dir, 'quality_v_time_per_domain.json'), 'w', encoding='utf-8') as f: 
        json.dump(domain_scatter, f, ensure_ascii=False, indent=4)
    with open(os.path.join(eval_dir, 'average_ci_v_si.json'), 'w', encoding='utf-8') as f: 
        json.dump(avg_cis, f, ensure_ascii=False, indent=4)

def main():
    REPO = setup.get_repo_base()
    BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
    REVISION_CACHE = os.environ.get("DOWNWARD_REVISION_CACHE")
    SEARCH_REVS = ["dawson-masters"]
    BUILD_OPTIONS = []
    DRIVER_OPTIONS = ["--overall-time-limit", "30m"]
    ENVIRONMENT = LocalEnvironment(processes=None)
    SUITE = setup.get_ipcs_sat_domains()

    CONFIGS = [
        IssueConfig("AWA*", ["--evaluator", "h=lmcut()", '--search',
            """eager_anytime(single(sum([weight(h, 2, verbosity=normal), g()])), 
            reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
        IssueConfig("e-AWA*", ["--evaluator", "h=lmcut()", "--search",
            """eager_anytime(epsilon_greedy(
            sum([weight(h, 2, verbosity=normal), g()]), epsilon=0.2, random_seed=1234), 
            reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
        IssueConfig("Type-AWA*", ["--evaluator", "h=lmcut()", "--search",
            """eager_anytime(alt(
            [single(weight(h, 2, verbosity=normal)), type_based([h, g()], random_seed=1234)]), 
            reopen_closed=true, f_eval=sum([h, g()]))"""], driver_options=DRIVER_OPTIONS),
        IssueConfig("RWA*", ["--evaluator", "h=lmcut()", "--search",
            """iterated([
                eager_wastar([h],w=5),
                eager_wastar([h],w=4),
                eager_wastar([h],w=3),
                eager_wastar([h],w=2),
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
    exp.add_parser(setup.DIR / "anytime_parser.py")
    # exp.add_parser(common_setup.DIR / "common_parser.py")
    exp.add_parser(exp.PLANNER_PARSER)

    if not setup.no_search():
        exp.add_step("build", exp.build)
        exp.add_step("start", exp.start_runs)

    exp.add_parse_again_step()
    exp.add_fetcher(name="fetch")
    exp.add_properties_processing_step({"anytime-experiment": _anytime_props_processor})
    exp.add_absolute_report_step(attributes=[
        Attribute("time:steps:avg", min_wins=True, absolute=True, function=arithmetic_mean), 
        Attribute("time:steps:optimal:avg", min_wins=True, function=arithmetic_mean),
        Attribute("change_index:first", min_wins=True, absolute=True, function=arithmetic_mean),
        Attribute("change_index:second", min_wins=True, absolute=True, function=arithmetic_mean),
        Attribute("change_index:third", min_wins=True, absolute=True, function=arithmetic_mean),
        Attribute("cost", min_wins=True, function=arithmetic_mean),
        "coverage", 
        Attribute("coverage:optimal", min_wins=True, absolute=True, function=finite_sum), 
        Attribute("expansions", min_wins=True, function=geometric_mean),
        Attribute("expansions:optimal", min_wins=True, function=geometric_mean),
        Attribute("planner_time:optimal", min_wins=True, function=geometric_mean)])


    # exp.add_comparison_table_step(attributes=["expansions"])
    # exp.add_scatter_plot_step(relative=True, attributes=["expansions"])

    exp.run_steps()



if __name__ == "__main__":
    main()