import re
from typing import List, Tuple
import itertools
from itertools import tee

def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)

def get_solution_change_indices(content):
    plan_regex = r"(sas_plan\.\d+) (\d+)"
    matches: List[Tuple[str, str]] = re.findall(plan_regex, content)
    converted_matches = [(f, float(c) ) for f, c in matches]
    first_diffs = []
    for s1, s2 in pairwise(converted_matches):
        with open(s1[0], 'r') as sol_1_f, open(s2[0], 'r') as sol_2_f:
            sol_1 = sol_1_f.readlines()
            sol_2 = sol_2_f.readlines()
            for i in range(len(sol_1)):
                if sol_1[i] != sol_2[i]:
                    first_diffs.append(i)
                    break

    return first_diffs


with open('run.log', 'r') as f:
    content = f.read()
    print(get_solution_change_indices(content))
