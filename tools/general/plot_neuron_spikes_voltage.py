from __future__ import annotations

import matplotlib.pyplot as plt
import numpy as np
import glob
import fileinput
import argparse
import sys
from pathlib import Path


def plot_neuron_on_time(path: Path, neuron_num: int) -> None:
    escaped_path = Path(glob.escape(path))  # type: ignore
    voltage_files = glob.glob(str(escaped_path / "*-voltage-gid=*.txt"))
    spikes_files = glob.glob(str(escaped_path / "*-spikes-gid=*.txt"))
    if not voltage_files:
        print(f"No valid voltage files have been found in path {path}", file=sys.stderr)
        exit(1)
    if not spikes_files:
        print(f"No valid spikes files have been found in path {path}", file=sys.stderr)
        exit(1)

    voltages = np.loadtxt(fileinput.input(voltage_files))
    spikes = np.loadtxt(fileinput.input(spikes_files))
    spikes = spikes.reshape((-1, 2))
    voltage_neuron = voltages[voltages[:, 0] == neuron_num]
    spikes_neuron = spikes[spikes[:, 0] == neuron_num]

    plt.plot(voltage_neuron[:, 1], voltage_neuron[:, 2])
    plt.scatter(spikes_neuron[:, 1], -0.02 * np.ones(spikes_neuron.shape[0]), color='r', marker='x')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--neuron', type=int, default=0, help='Neuron to examine (default: 0)')
    args = parser.parse_args()

    plot_neuron_on_time(args.path, args.neuron)
    plt.show()
