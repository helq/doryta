from __future__ import annotations

import matplotlib.pyplot as plt
import numpy as np


if __name__ == '__main__':
    neuron_num = 0
    voltages = np.loadtxt("build/output/five-neurons-test-voltage-gid=0.txt")  # type: ignore
    spikes = np.loadtxt("build/output/five-neurons-test-spikes-gid=0.txt")  # type: ignore
    spikes = spikes.reshape((-1, 2))
    voltage_neuron = voltages[voltages[:, 0] == neuron_num]
    spikes_neuron = spikes[spikes[:, 0] == neuron_num]
    plt.plot(voltage_neuron[:, 1], voltage_neuron[:, 2])
    plt.scatter(spikes_neuron[:, 1], -0.02 * np.ones(spikes_neuron.shape[0]), color='r', marker='x')
    plt.show()
