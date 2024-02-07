#! /usr/bin/env python3

from datetime import datetime
import multiprocessing
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
    ENVIRONMENT = LocalEnvironment(processes=multiprocessing.cpu_count()-1)
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

    def _get_lama_first():
        return [
            "--search",
            "--if-unit-cost",
            "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=true), let(hff, ff(), lazy_greedy([hff,hlm],preferred=[hff,hlm]) ))",
            "--if-non-unit-cost",
            "let(hlm1, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),transform=adapt_costs(one),pref=true), let(hff1, ff(transform=adapt_costs(one)), lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1], cost_type=one,reopen_closed=false) ))"
        ]
    
    def _get_lama_configs():
        FF_RANDOM_SEED = int(datetime.now().timestamp())
        return [
            IssueConfig('Softmin-lama', ["--evaluator", 
                        "hlm=landmark_sum(lm_factory=lm_rhw(use_orders=true),transform=adapt_costs(one),pref=false)",
                        "--evaluator", 
                        "hff=ff(transform=adapt_costs(one))",
                        "--search", f"lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),softmin_type_based([hff,g()], random_seed={FF_RANDOM_SEED})],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false)"] , driver_options=DRIVER_OPTIONS),
            IssueConfig('Type-lama', ["--evaluator", 
                        "hlm=landmark_sum(lm_factory=lm_rhw(use_orders=true),transform=adapt_costs(one),pref=false)",
                        "--evaluator", 
                        "hff=ff(transform=adapt_costs(one))",
                        "--search", f"lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true), type_based([hff, g()], random_seed={FF_RANDOM_SEED})],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false)"] , driver_options=DRIVER_OPTIONS),
            IssueConfig('HI-lama', ["--evaluator", 
                        "hlm=landmark_sum(lm_factory=lm_rhw(use_orders=true),transform=adapt_costs(one),pref=false)",
                        "--evaluator", 
                        "hff=ff(transform=adapt_costs(one))",
                        "--search", f"lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true), hib_partition(hff, biased_depth(tau=1, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}))],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false)"] , driver_options=DRIVER_OPTIONS),
            IssueConfig('LWM-lama', ["--evaluator", 
                        "hlm=landmark_sum(lm_factory=lm_rhw(use_orders=true),transform=adapt_costs(one),pref=false)",
                        "--evaluator", 
                        "hff=ff(transform=adapt_costs(one))",
                        "--search", f"lazy(alt([single(hff),single(hff,pref_only=true),single(hlm),single(hlm,pref_only=true),lwmb_partition(hff, biased_depth(tau=1, random_seed={FF_RANDOM_SEED}), intra_biased(random_seed={FF_RANDOM_SEED}))],boost=1000),preferred=[hff,hlm],cost_type=one,reopen_closed=false)"] , driver_options=DRIVER_OPTIONS),
        ]

    normal_ff = "ff()"
    unit_ff = "ff(transform=adapt_costs(cost_type=one))"
    unit_cost = 'cost_type=one'
    real_cost = 'cost_type=normal'

    FF_RANDOM_SEED = int(datetime.now().timestamp())
    # CONFIGS = [
               
        # IssueConfig('Softmin', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt( [ single(h), softmin_type_based([h, g()], ignore_size=true, random_seed={FF_RANDOM_SEED}) ] ), {unit_cost})  '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('HITS', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt( [ single(h), hib_partition(h, biased_depth(tau=1, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED})) ] ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        
    # ]
    CONFIGS = _get_lama_configs()

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


