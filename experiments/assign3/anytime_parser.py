#! /usr/bin/env python

"""
Custom Parser for anytime-search runs of Fast Downward.
"""

from itertools import tee
import re
from typing import List, Tuple

from lab.parser import Parser


def _get_states_pattern(attribute, name):
    return (attribute, rf"{name} (\d+) state\(s\)\.", int)

def find_all_matches(attribute, regex, type=int):
    """
    Look for all occurences of *regex*, cast what is found in brackets to
    *type* and store the list of found items in the properties dictionary
    under *attribute*. *regex* must contain exactly one bracket group.
    """

    def store_all_occurences(content, props):
        matches = re.findall(regex, content)
        props[attribute] = [type(m) for m in matches]

    return store_all_occurences

def get_solution_timestamp_steps(time_step, total_time):

    def store_all_timestamp_steps(content, props):

        start_match = re.search(r"Start Timestep: (.+) millisecond\(s\).\n", content).group(1)
        end_match = re.findall(r"Timestamp: (.+) millisecond\(s\).\n", content)[-1]
        matches = re.findall(r"Solution Timestep: (.+) millisecond\(s\).\n", content)
        converted_matches = [int(start_match)]
        converted_matches = [int(m) for m in matches]
        converted_matches.append(int(end_match))
        print(converted_matches)
        steps = []
        abs_time = []
        for i in range(len(converted_matches)-1):
            steps.append(converted_matches[i+1] - converted_matches[i])
            abs_time.append(converted_matches[i+1] - converted_matches[0])

        props[time_step] = steps
        props[total_time] = abs_time
        
    
    return store_all_timestamp_steps


def get_solution_change_indices(indices_prop):
    def pairwise(iterable):
        "s -> (s0,s1), (s1,s2), (s2, s3), ..."
        a, b = tee(iterable)
        next(b, None)
        return zip(a, b)

    def store_solution_change_indices(content, props):
        plan_regex = r"(sas_plan\.\d+) (\d+)"
        matches: List[Tuple[str, str]] = re.findall(plan_regex, content)
        converted_matches = [(f, float(c) ) for f, c in matches]
        first_diffs = []
        for s1, s2 in pairwise(converted_matches):
            with open(s1[0], 'r') as sol_1_f, open(s2[0], 'r') as sol_2_f:
                sol_1 = sol_1_f.readlines()
                sol_2 = sol_2_f.readlines()
                lim = len(sol_2) if len(sol_2) < len(sol_1) else len(sol_1)
                for i in range(lim):
                    if sol_1[i] != sol_2[i]:
                        first_diffs.append(i)
                        break         
        props[indices_prop] = first_diffs

    return store_solution_change_indices



def reduce_to_min(list_name, single_name):
    def reduce_to_minimum(content, props):
        values = props.get(list_name, [])
        if values:
            props[single_name] = min(values)

    return reduce_to_minimum

def reduce_to_max(list_name, single_name):
    def reduce_to_maximum(content, props):
        values = props.get(list_name, [])
        if values:
            props[single_name] = max(values)

    return reduce_to_maximum


def coverage(content, props):
    props["coverage"] = int("cost" in props)


def add_memory(content, props):
    """Add "memory" attribute if the run was not aborted.

    Peak memory usage is printed even for runs that are terminated
    abnormally. For these runs we do not take the reported value into
    account since the value is censored: it only takes into account the
    memory usage until termination.

    """
    raw_memory = props.get("raw_memory")
    if raw_memory is not None:
        if raw_memory < 0:
            props.add_unexplained_error("planner failed to log peak memory")
        elif props["coverage"]:
            props["memory"] = raw_memory


def main():
    parser = Parser()
    parser.add_pattern("raw_memory", r"Peak memory: (.+) KB", type=int),
    parser.add_function(find_all_matches("cost:all", r"Plan cost: (.+)\n", type=float))
    parser.add_function(
        find_all_matches("steps:all", r"Plan length: (.+) step\(s\).\n", type=int)
    )
    parser.add_function(
        find_all_matches("cost:all", r"Plan cost: (.+)\n", type=float)
    )

    # indicate if optimal solution was found
    parser.add_pattern("optimal:found", r"Optimal Solution: (.+)", type=str)

    # timestamps
    parser.add_function(
        get_solution_timestamp_steps("time:steps", "time:total")
    )

    # solution first change index
    parser.add_function(
        get_solution_change_indices("change_indices")
    )

    #expansions
    exp, exp_re, exp_type = _get_states_pattern("expansions:all", "Expanded")
    parser.add_function(find_all_matches(exp, exp_re, type=exp_type))
    parser.add_function(reduce_to_max("expansions:all", "expansions"))

    #generated
    gen, gen_re, gen_type = _get_states_pattern("generated:all", "Generated")
    parser.add_function(find_all_matches(gen, gen_re, type=gen_type))
    parser.add_function(reduce_to_max("generated:all", "generated"))

    parser.add_function(reduce_to_min("cost:all", "cost"))
    parser.add_function(reduce_to_min("steps:all", "steps"))
    
    parser.add_function(coverage)
    parser.add_function(add_memory)
    # todo: add functions to get averages and whatnot (see coverage function)
    
    parser.parse()


main()
