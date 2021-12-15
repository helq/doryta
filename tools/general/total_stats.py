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
from ast import literal_eval
import csv

from typing import Tuple, List, Optional
from collections import namedtuple


Stats = namedtuple('Stats', ['leaks', 'integrations', 'fires'])


def aggregate_stats(
    path: pathlib.Path,
    groups: Optional[List[int]] = None
) -> Tuple[Stats, List[Tuple[str, Stats]]]:
    stat_files = glob(str(path / "*-stats-gid=*.txt"))
    if not stat_files:
        print(f"No valid stats files have been found in path {path}", file=sys.stderr)
        exit(1)

    stats_per_neuron = np.loadtxt(fileinput.input(stat_files), dtype=int)  # type: ignore
    assert(len(stats_per_neuron.shape) == 2)
    assert(stats_per_neuron.shape[1] == 4)

    grouped: List[Tuple[str, Stats]] = []
    if groups:
        j = 0
        for i in groups:
            neuron_range = np.bitwise_and(j <= stats_per_neuron[:, 0],
                                          stats_per_neuron[:, 0] < i)
            res = stats_per_neuron[neuron_range, :].sum(axis=0)
            grouped.append((f"[{j}:{i-1}]", Stats(*res[1:])))
            j = i
        res = stats_per_neuron[stats_per_neuron[:, 0] >= j].sum(axis=0)
        last_neuron = stats_per_neuron[:, 0].max()
        grouped.append((f"[{j}:{last_neuron}]", Stats(*res[1:])))

    res = stats_per_neuron.sum(axis=0)

    return Stats(*res[1:]), grouped


def print_operations(stats: Stats) -> None:
    print("leaks =", stats.leaks)
    print("integrations =", stats.integrations)
    print("fires =", stats.fires)


def stats_div_by(stats: Stats, num: int) -> Stats:
    return Stats(stats.leaks / num,
                 stats.integrations / num,
                 stats.fires / num)


def print_stats(stats: Stats, num: Optional[int]) -> None:
    if num:
        print("Average stats:")
        print_operations(stats_div_by(stats, num))
    else:
        print("Acculated stats:")
        print_operations(stats)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=pathlib.Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--iterations', type=int,
                        help='Total number of iterations the network was run'
                        ' (eg, 20 - total number of inferences)', default=None)
    parser.add_argument('--groups', type=literal_eval,
                        help='Indices to group the aggregated stats'
                        ' (eg, [10, 15] - aggregate stats into three groups 0-10, 11-15, 16-end)',
                        default=None)
    parser.add_argument('--csv', type=pathlib.Path, default=None,
                        help='Save statistics as a csv file (no extension)')
    args = parser.parse_args()

    if args.groups is not None:
        if not (isinstance(args.groups, list)
                and all(isinstance(e, int) for e in args.groups)):
            print("Error: groups should be a list containing only integers", file=sys.stderr)
            exit(1)

    total, grouped = aggregate_stats(args.path, args.groups)
    print("Stats for entire circuit")
    print_stats(total, args.iterations)

    for i, (group_name, stats_i) in enumerate(grouped):
        print(f"\nGroup {i} - {group_name}")
        print_stats(stats_i, args.iterations)

    if args.csv is not None:
        filename = args.csv.with_name(
            args.csv.name + ('-average.csv' if args.iterations is not None
                             else '-acculated.csv'))

        with open(filename, 'w', newline='') as csvfile:
            n = args.iterations
            csvwriter = csv.writer(csvfile, quoting=csv.QUOTE_MINIMAL)

            csvwriter.writerow(["name-unit", "leak", "integration", "fire"])
            csvwriter.writerow(["total"] + list(total if n is None
                                                else stats_div_by(total, n)))
            for group_name, stats_i in grouped:
                csvwriter.writerow([group_name] +
                                   list(stats_i if n is None
                                        else stats_div_by(stats_i, n)))


# To compute parameters of Fully Connected MNIST
# python tools/general/total_stats.py --path build/output-all-stats \
#   --iterations 10000 --groups '[784,1040,1104]' \
#   --csv fully-connected-mnist
# For game of life:
# python ../../tools/general/total_stats.py --path . --iterations=160 \
#    --groups '[400,800]' --csv ../../gol-die-hard-80-iterations
