from __future__ import annotations

from typing import Any, Tuple, Union

import numpy as np
import fileinput
from glob import glob


def extracting_inference_from_doryta_spikes(path: str) -> Any:
    shift = 28*28 + 256 + 64  # Ugly, hardcoded, but there is only one model so far

    spikes = np.loadtxt(fileinput.input(glob(f"{path}/*-spikes-gid=*.txt")))  # type: ignore
    spikes = spikes.reshape((-1, 2)).astype(int)

    num_imgs = spikes[:, 1].max() + 1

    # Finding output neurons (the last in the list)
    output_neurons = np.flatnonzero(shift <= spikes[:, 0])
    spikes = spikes[output_neurons]
    spikes[:, 0] -= shift

    # Sorting is necessary to separate spikes by time stamps using the two lines below
    spikes = spikes[spikes[:, 1].argsort()]
    time_stamps, indices = np.unique(spikes[:, 1], return_index=True)  # type: ignore
    spikes_per_class = np.array(np.split(spikes[:, 0], indices[1:]), dtype=object)  # type: ignore

    find_max = np.vectorize(lambda x: np.argmax(np.bincount(x // 10)))
    inferenced = - np.ones((num_imgs,), dtype=int)
    inferenced[time_stamps] = find_max(spikes_per_class)
    return inferenced, time_stamps, spikes_per_class


def get_real_classification(path: str) -> Any:
    return np.fromfile(path, dtype='b')  # type: ignore


# Copied from data/models/whetstone/code
def load_data(
) -> Tuple[Tuple[np.ndarray, np.ndarray], Tuple[np.ndarray, np.ndarray]]:  # type: ignore
    from tensorflow.keras.datasets import mnist
    from tensorflow.keras.utils import to_categorical

    # Loading and preprocessing data
    numClasses = 10
    (x_train, y_train), (x_test, y_test) = mnist.load_data()

    x_train = x_train.astype('float32')
    x_train /= 255
    x_test = x_test.astype('float32')
    x_test /= 255

    y_train = to_categorical(y_train, numClasses)
    y_test = to_categorical(y_test, numClasses)

    x_train = np.reshape(x_train, (60000, 28*28))
    x_test = np.reshape(x_test, (10000, 28*28))
    return (x_train, y_train), (x_test, y_test)


def prediction_using_whetstone(model_path: str) -> Any:
    from tensorflow.keras.models import load_model

    model = load_model(model_path)
    (x_train, y_train), (x_test, y_test) = load_data()
    imgs = (x_test[...] > .5).astype(float)
    return model.predict(imgs).argmax(axis=1)


if __name__ == '__main__':
    path_results = "build/output-all"
    path_real_tags = "data/models/whetstone/spikified-mnist/spikified-images-all.tags.bin"
    path_to_keras_model = "data/models/whetstone/keras-simple-mnist"

    inferenced, time_stamps, spikes_per_class = \
        extracting_inference_from_doryta_spikes(path_results)
    print("Spike inference:", inferenced)

    test_values = get_real_classification(path_real_tags)
    assert(inferenced.size == test_values.size)
    print("Total correctly inferenced:", (inferenced == test_values).sum())

    whetstone_prediction = prediction_using_whetstone(path_to_keras_model)
    print("Total differently classified:", (inferenced != whetstone_prediction).sum())

    different_classification = np.argwhere((inferenced != whetstone_prediction)).flatten()
    print("Ten first differently classified images:", different_classification[:10])
