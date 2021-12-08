"""
Given the stats for individual neurons, this script loads them and computes the total and
average of the stats.
"""

from __future__ import annotations

import numpy as np
from glob import glob
import fileinput
import argparse
import sys
import pathlib

from typing import Tuple


def aggregate_stats(path: pathlib.Path) -> Tuple[int, int, int]:
    stat_files = glob(str(path / "*-stats-gid=*.txt"))
    if not stat_files:
        print(f"No valid stats files have been found in path {path}", file=sys.stderr)
        exit(1)

    stats_per_neuron = np.loadtxt(fileinput.input(stat_files))  # type: ignore
    assert(len(stats_per_neuron.shape) == 2)
    assert(stats_per_neuron.shape[1] == 4)

    res = stats_per_neuron.astype(int).sum(axis=0)

    return res[1], res[2], res[3]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=pathlib.Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--iterations', type=int,
                        help='Total number of iterations the network was run'
                        ' (eg, 20 - total number of inferences)', default=None)
    # parser.add_argument('--groups', type=int,
    #                     help='Indices to group the aggregated stats'
    #                     ' (eg, [10, 15] - aggregate stats into three groups 0-10, 11-15, 16-end)',
    #                     default=None)
    args = parser.parse_args()

    leaks, integrations, fires = aggregate_stats(args.path)
    print("Total stats:")
    print("leaks =", leaks)
    print("integrations =", integrations)
    print("fires =", fires)

    if args.iterations:
        print("\nAverage stats:")
        print("leaks =", leaks / args.iterations)
        print("integrations =", integrations / args.iterations)
        print("fires =", fires / args.iterations)
