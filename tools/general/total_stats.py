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

from typing import Tuple, List, Optional, NamedTuple


class Stats(NamedTuple):
    out_synapses: int
    leaks: float
    integrations: float
    fires: float


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
    assert(stats_per_neuron.shape[1] == 5)

    last_neuron = stats_per_neuron[:, 0].max()
    grouped: List[Tuple[str, Stats]] = []
    if groups:
        j = 0
        for i in groups:
            neuron_range = np.bitwise_and(j <= stats_per_neuron[:, 0],
                                          stats_per_neuron[:, 0] < j + i)
            res = stats_per_neuron[neuron_range, :].sum(axis=0)
            avg = np.average(stats_per_neuron[neuron_range, 1])  # type: ignore
            grouped.append((f"[{j}:{j + i-1}]", Stats(avg, *res[2:])))
            j += i
        neuron_range = stats_per_neuron[:, 0] >= j
        res = stats_per_neuron[neuron_range].sum(axis=0)
        avg = np.average(stats_per_neuron[neuron_range, 1])  # type: ignore
        grouped.append((f"[{j}:{last_neuron}]", Stats(avg, *res[2:])))

    avg = np.average(stats_per_neuron[:, 1])  # type: ignore
    res = stats_per_neuron.sum(axis=0)

    return Stats(avg, *res[2:]), grouped


def print_operations(stats: Stats) -> None:
    print("out-synapses (per neuron) =", stats.out_synapses)
    print("leaks =", stats.leaks)
    print("integrations =", stats.integrations)
    print("fires =", stats.fires)


def stats_div_by(stats: Stats, num: int) -> Stats:
    return Stats(stats.out_synapses,
                 stats.leaks / num,
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

            csvwriter.writerow(["name-unit", "out-synapses", "leak", "integration", "fire"])
            csvwriter.writerow(["total"] + list(total if n is None  # type: ignore
                                                else stats_div_by(total, n)))
            for group_name, stats_i in grouped:
                csvwriter.writerow([group_name] +
                                   list(stats_i if n is None  # type: ignore
                                        else stats_div_by(stats_i, n)))


# To compute parameters of Fully Connected MNIST
# python tools/general/total_stats.py --path build/fully-all-stats \
#   --iterations 10000 --groups '[784,256,64]' \
#   --csv fully-connected-mnist
# For game of life:
# python ../../tools/general/total_stats.py --path gol-die-hard-80-steps --iterations=160 \
#    --groups '[400,400]' --csv gol-die-hard-80-iterations
# For LeNet:
# python tools/general/total_stats.py --path build_novisualcode/lenet-filters=32,48-1000 \
#    --iterations 1000 --csv lenet-filters=32,48-with-1000 \
#    --groups '[784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,120,84]' # noqa
# python tools/general/total_stats.py --path build/lenet-filters=6,16-all \
#    --iterations 10000 --csv lenet-filters=32,48-all \
#    --groups '[784,784,784,784,784,784,784,196,196,196,196,196,196,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,120,84]' # noqa
