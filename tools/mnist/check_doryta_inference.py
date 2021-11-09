from __future__ import annotations

from typing import Any, Tuple

import numpy as np
import fileinput
from glob import glob
import sys


def extracting_inference_from_doryta_spikes(path: str) -> Tuple[Any, Any, Any, Any]:
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
    # inferenced = - np.ones((num_imgs,), dtype=int)
    inferenced = np.zeros((num_imgs,), dtype=int)
    inferenced[time_stamps] = find_max(spikes_per_class)
    return inferenced, spikes, time_stamps, spikes_per_class


def get_real_classification(path: str) -> Any:
    return np.fromfile(path, dtype='b')  # type: ignore


def prediction_using_whetstone(model_path: str, indices: slice) -> Tuple[Any, Any]:
    # Yeah, this is ugly, very ugly, but it's better than duplicating code
    import importlib.util
    import os
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'  # this suppresses Keras/tf warnings
    # This is to be able to import the Simple MNIST script as a module
    spec = importlib.util.spec_from_file_location(
        "snn_models.whetstone.simple_mnist",
        "data/models/whetstone/code/modified-simple-mnist.py")
    # This is to be able to import whetsone
    whetstone_path = "data/models/whetstone/code/"
    if whetstone_path not in sys.path:
        sys.path.append(whetstone_path)
    if spec is None:
        raise Exception("Not able to load module where simple MNIST whetstone model is defined")

    simple_mnist = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(simple_mnist)  # type: ignore
    model, model_intermediate = simple_mnist.load_models(model_path)  # type: ignore

    (x_train, y_train), (x_test, y_test) = simple_mnist.load_data()  # type: ignore
    imgs = (x_test[indices] > .5).astype(float)
    return model.predict(imgs).argmax(axis=1), model_intermediate.predict(imgs)


if __name__ == '__main__':
    path_to_keras_model = "data/models/whetstone/keras-simple-mnist"
    path_real_tags = "data/models/whetstone/spikified-mnist/spikified-images-all.tags.bin"

    # indices_test = slice(950, 970)
    # path_results = "build/output-950-to-970-AIMOS-trial=6"
    indices_test = slice(10000)
    path_results = "build/output-all"

    print(f"Execution path being analyzed: `{path_results}`")

    # ====== Extracting inference results from Doryta run ======
    inferenced, spikes, time_stamps, spikes_per_class = \
        extracting_inference_from_doryta_spikes(path_results)
    print("Doryta inference labels:", inferenced)

    # ====== Finding the classification LABELS from dataset ======
    test_values = get_real_classification(path_real_tags)
    test_values = test_values[indices_test]
    assert(inferenced.size == test_values.size)
    print("Total correctly inferenced (Doryta):", (inferenced == test_values).sum())

    # ====== Using Keras/Whetstone to classify data ======
    whetstone_prediction, whetstone_prediction_inter = \
        prediction_using_whetstone(path_to_keras_model, indices_test)

    # ====== Checking discrepancies between Keras and Doryta ======
    assert(whetstone_prediction.size == test_values.size)
    print("Discrepancy between Doryta and Whetstone:", (inferenced != whetstone_prediction).sum())

    different_classification = np.argwhere((inferenced != whetstone_prediction)).flatten()
    print("Image IDs for the first 20 discrepant images:", different_classification[:20] +
          (indices_test.start
           if isinstance(indices_test, slice) and indices_test.start
           else 0))

    sz_imgs = indices_test.stop - (0 if indices_test.start is None else indices_test.start)
    spikes_as_ones = np.zeros((sz_imgs, 100))
    spikes_as_ones.flat[spikes[:, 0] + spikes[:, 1] * 100] = 1  # type: ignore

    pred_spikes_as_ones = whetstone_prediction_inter[-1]

    print("Do all output spikes coincide between doryta and keras?",
          np.all(pred_spikes_as_ones == spikes_as_ones))

    doryta_spikes = spikes_as_ones.astype(bool)
    keras_spikes = pred_spikes_as_ones.astype(bool)
    only_in_doryta = np.argwhere(np.bitwise_and(np.bitwise_not(keras_spikes), doryta_spikes))
    only_in_keras = np.argwhere(np.bitwise_and(np.bitwise_not(doryta_spikes), keras_spikes))
    print("Total spikes only in doryta:", only_in_doryta.size)
    print("Total spikes only in keras/whetstone:", only_in_keras.size)
