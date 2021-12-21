from __future__ import annotations

import numpy as np
from glob import glob
import fileinput
import sys
import pathlib
import argparse

from typing import Any


def collect_stats(
    path: pathlib.Path
) -> np.ndarray[Any, Any]:
    stat_files = glob(str(path / "*-stats-gid=*.txt"))
    if not stat_files:
        print(f"No valid stats files have been found in path {path}", file=sys.stderr)
        exit(1)

    stats_per_neuron = np.loadtxt(fileinput.input(stat_files), dtype=int)  # type: ignore

    return stats_per_neuron  # type: ignore


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=pathlib.Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--save', type=pathlib.Path, help='Directory to save aggregated stats',
                        required=True)
    args = parser.parse_args()

    stats = collect_stats(args.path)

    n_neurons = stats[:, 0].max(axis=0) + 1
    aggregated = np.zeros((n_neurons, stats.shape[1]))
    for i in range(n_neurons):
        stats_i = stats[stats[:, 0] == i]
        aggregated[i, 2:] = stats_i.sum(axis=0)[2:]
        aggregated[i, :2] = stats_i[0, :2]

    np.savetxt(str(args.save / 'aggregated-stats-gid=0.txt'), aggregated, fmt='%d')  # type: ignore
