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

    normal_ff = "ff()"
    unit_ff = "ff(transform=adapt_costs(cost_type=one))"
    unit_cost = 'cost_type=one'
    real_cost = 'cost_type=normal'
    # ff='ff()'
    FF_RANDOM_SEED = int(datetime.now().timestamp())
    CONFIGS = [
        
        # IssueConfig('HI', ["--evaluator", f"h={unit_ff}", '--search', f'eager(hi_partition(h, inter_uniform(random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM', ["--evaluator", f"h={unit_ff}", '--search', f'eager(lwm_partition(h, inter_uniform(random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('[h]-path', ["--evaluator", f"h={unit_ff}", '--search', f'eager(type_based_path([h], random_seed={FF_RANDOM_SEED} ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('[h,g]', ["--evaluator", f"h={unit_ff}", '--search', f'eager(type_based([h,g()], random_seed={FF_RANDOM_SEED} ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS)

        # IssueConfig('LWM-05', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.05, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM-10', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.1, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM-20', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.2, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM-30', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.3, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM-50', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.25, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 
        # IssueConfig('LWM-75', ["--evaluator", f"h={ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.75, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}) )) '] , driver_options=DRIVER_OPTIONS), 

        # IssueConfig('[h,g]', ["--evaluator", f"h={ff}", '--search', f'eager(type_based([h,g()], random_seed={FF_RANDOM_SEED} )) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('[h]', ["--evaluator", f"h={ff}", '--search', f'eager(type_based([h], random_seed={FF_RANDOM_SEED} )) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('[h]-path', ["--evaluator", f"h={ff}", '--search', f'eager(type_based_path([h], random_seed={FF_RANDOM_SEED} )) '] , driver_options=DRIVER_OPTIONS)

        # IssueConfig('Softmin-Type', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt([single(h), softmin_type_based([h, g()], ignore_size=true, random_seed={FF_RANDOM_SEED})]), {unit_cost})'] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('Asai', ['--evaluator', f"h={unit_ff}", '--evaluator', f'r=random_edge(random_seed={FF_RANDOM_SEED})', '--search', f'eager(alt( [tiebreaking( [h, r] ), single(r)]), {unit_cost} )'], driver_options=DRIVER_OPTIONS),
        # IssueConfig('Fan-Type', ['--evaluator', f"h={unit_ff}", '--search', f'eager(alt( [ single(h), type_based([h, g()], random_seed={FF_RANDOM_SEED}) ] ), {unit_cost}) '], driver_options=DRIVER_OPTIONS),
        # IssueConfig('LWM-unit-0.2929/0.2929', ["--evaluator", f"h={unit_ff}", '--search', f'eager(lwm_partition(h, inter_ep_minh(h, 0.2929, random_seed={FF_RANDOM_SEED}), intra_ep_minh(h, 0.2929, random_seed={FF_RANDOM_SEED}) ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('LWM-real-0.2929/0.2929', ["--evaluator", f"h={normal_ff}", '--search', f'eager(hi_partition(h, inter_ep_minh(h, 0.2929, random_seed={FF_RANDOM_SEED}), intra_ep_minh(h, 0.2929, random_seed={FF_RANDOM_SEED}) ), {real_cost}) '] , driver_options=DRIVER_OPTIONS),
        
        # IssueConfig('Softmin-Type', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt([single(h), softmin_type_based([h, g()], ignore_size=true, random_seed={FF_RANDOM_SEED})]), {unit_cost})'] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('LWM-unit-biased-depth (Kuroiwa config)', ["--evaluator", f"h={unit_ff}", '--search', f'eager(alt([single(h), lwm_partition(h, inter_biased_depth(ignore_size=true, random_seed={FF_RANDOM_SEED}), intra_uniform(random_seed={FF_RANDOM_SEED}))]), cost_type=one)'] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('HI-unit-biased-minh (Kuroiwa config)', ["--evaluator", f"h={unit_ff}", '--search', f'eager( ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('HI-unit-biased-minh/0.2', ["--evaluator", f"h={unit_ff}", '--search', f'eager(hi_partition(h, inter_biased_minh(h, ignore_size=true, random_seed={FF_RANDOM_SEED}), intra_ep_minh(h, 0.2, random_seed={FF_RANDOM_SEED}) ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        # IssueConfig('HI-unit-biased-root/biased', ["--evaluator", f"h={unit_ff}", '--search', f'eager(hi_partition(h, inter_biased_root(h, ignore_size=true, random_seed={FF_RANDOM_SEED}), intra_biased_minh(h, ignore_size=true, random_seed={FF_RANDOM_SEED}) ), {unit_cost}) '] , driver_options=DRIVER_OPTIONS)

        # IssueConfig('HI-bias/bias', ["--evaluator", f"h={unit_ff}", '--search', f'eager(hi_partition(h, inter_biased_depth(ignore_size=true, random_seed={FF_RANDOM_SEED}), intra_biased(h, ignore_size=true, random_seed={FF_RANDOM_SEED})), {unit_cost}) '] , driver_options=DRIVER_OPTIONS),
        IssueConfig('LWM-bias/bias', ["--evaluator", f"h={unit_ff}", '--search', f'eager(lwm_partition(h, inter_biased_depth(tau=3, ignore_size=true, random_seed={FF_RANDOM_SEED}), intra_biased(h, ignore_size=true, random_seed={FF_RANDOM_SEED})), {unit_cost}) '] , driver_options=DRIVER_OPTIONS)
    
    ]       

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


