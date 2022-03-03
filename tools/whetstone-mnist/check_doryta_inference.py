from __future__ import annotations

from typing import Any, Tuple, List

import glob
from pathlib import Path
import argparse
import fileinput
import sys
from enum import Enum

import numpy as np
from numpy.typing import NDArray


class ModelType(Enum):
    Fully = 0
    LeNet = 1


def extracting_inference_from_doryta_spikes(
    path: Path,
    shift: int,
    total_output_neurons: int = 100
) -> Tuple[Any, Any, Any, Any]:
    escaped_path = Path(glob.escape(path))  # type: ignore
    spikes_glob = glob.glob(str(escaped_path / "*-spikes-gid=*.txt"))
    if not spikes_glob:
        print(f"No output spikes found in the directory {path}", file=sys.stderr)
        exit(1)

    spikes = np.loadtxt(fileinput.input(spikes_glob))
    spikes = spikes.reshape((-1, 2)).astype(int)

    num_imgs = spikes[:, 1].max() + 1

    # Finding output neurons (the last in the list)
    output_neurons = np.flatnonzero(
        np.bitwise_and(shift <= spikes[:, 0], spikes[:, 0] < shift + total_output_neurons))
    spikes = spikes[output_neurons]
    spikes[:, 0] -= shift

    # Sorting is necessary to separate spikes by time stamps using the two lines below
    spikes = spikes[spikes[:, 1].argsort()]
    time_stamps, indices = np.unique(spikes[:, 1], return_index=True)  # type: ignore
    spikes_per_img: List[NDArray[Any]] = np.split(spikes[:, 0], indices[1:])
    inferenced: NDArray[Any] = np.zeros((num_imgs,), dtype=int)

    for stamp, spikes_i in zip(time_stamps, spikes_per_img):
        inferenced[stamp] = np.argmax(np.bincount(spikes_i // 10))

    return inferenced, spikes, time_stamps, spikes_per_img


def get_real_classification(path: Path) -> Any:
    return np.fromfile(path, dtype='b')


def prediction_using_whetstone(
    model_path: Path,
    indices: slice,
    model_type: ModelType,
    dataset: str = 'mnist'
) -> Tuple[Any, Any]:
    import os
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'  # this suppresses Keras/tf warnings
    from ws_models.common_mnist import load_data
    if model_type == ModelType.Fully:
        # ws_models is a symbolic link to the directory where the whetstone model is defined
        from ws_models.ffsnn_mnist import load_models
    elif model_type == ModelType.LeNet:
        from ws_models.lenet_mnist import load_models  # type: ignore

    model, model_intermediate = load_models(model_path)

    (x_train, y_train), (x_test, y_test) = load_data(dataset)
    if model_type == ModelType.LeNet:
        x_train = x_train.reshape((-1, 28, 28, 1))
        x_test = x_test.reshape((-1, 28, 28, 1))

    imgs = (x_test[indices] > .5).astype(float)

    return model.predict(imgs).argmax(axis=1), model_intermediate.predict(imgs)


def check_doryta_output_to_keras(
    path_to_keras_model: Path,
    path_real_tags: Path,
    indices_test: slice,
    doryta_output: Path,
    shift: int,
    which: ModelType,
    dataset: str
) -> None:
    print(f"Execution path being analyzed: `{doryta_output}`")

    # ====== Extracting inference results from Doryta run ======
    inferenced, spikes, time_stamps, spikes_per_img = \
        extracting_inference_from_doryta_spikes(doryta_output, shift)
    print("Doryta inference labels:", inferenced)

    # ====== Finding the classification LABELS from dataset ======
    test_values = get_real_classification(path_real_tags)
    test_values = test_values[indices_test]
    assert inferenced.size == test_values.size, \
        f"inferenced.size = {inferenced.size} and test_values.size = {test_values.size}"
    print("Total correctly inferenced (Doryta):",
          (inferenced == test_values).sum(),
          "/", inferenced.size)

    # ====== Using Keras/Whetstone to classify data ======
    whetstone_prediction, whetstone_prediction_inter = \
        prediction_using_whetstone(path_to_keras_model, indices_test, which, dataset)

    # ====== Checking discrepancies between Keras and Doryta ======
    assert(whetstone_prediction.size == test_values.size)
    discrepancy = (inferenced != whetstone_prediction).sum()

    # import code
    # locls = globals().copy()
    # locls.update(locals())
    # code.InteractiveConsole(locals=locls).interact()

    if discrepancy:
        print("Discrepancy between Doryta and Whetstone:",
              discrepancy)

        different_classification = np.argwhere((inferenced != whetstone_prediction)).flatten()
        print("Image IDs for the first 20 discrepant images:",
              different_classification[:20] +
              (indices_test.start
               if isinstance(indices_test, slice) and indices_test.start
               else 0))
    else:
        print("Doryta inferenced classes correspond to whetstone's")

    sz_imgs = indices_test.stop - (0 if indices_test.start is None else indices_test.start)
    spikes_as_ones = np.zeros((sz_imgs, 100))
    spikes_as_ones.flat[spikes[:, 0] + spikes[:, 1] * 100] = 1

    pred_spikes_as_ones = whetstone_prediction_inter[-1]

    doryta_same_as_keras = np.all(pred_spikes_as_ones == spikes_as_ones)
    print("Do all output spikes coincide between doryta and keras?",
          doryta_same_as_keras)

    if doryta_same_as_keras:
        print("Doryta produces the same ouput as keras! Awesome.")
    else:
        doryta_spikes: Any = spikes_as_ones.astype(bool)
        keras_spikes = pred_spikes_as_ones.astype(bool)
        only_in_doryta = np.argwhere(
            np.bitwise_and(np.bitwise_not(keras_spikes), doryta_spikes))
        only_in_keras = np.argwhere(
            np.bitwise_and(np.bitwise_not(doryta_spikes), keras_spikes))
        print("Total spikes only in doryta:", len(only_in_doryta))
        print("Total spikes only in keras/whetstone:", len(only_in_keras))


if __name__ == '__main__':
    path_to_keras_model = Path("data/models/whetstone/keras-simple-mnist")
    path_real_tags = Path("data/models/whetstone/spikified-mnist/spikified-images-all.tags.bin")

    indices_test = 20
    doryta_output = Path("build/fully-20")

    parser = argparse.ArgumentParser()
    parser.add_argument('--path-to-keras-model', type=Path, default=path_to_keras_model,
                        help='Place where keras/whetstone model is stored '
                        f'(default: {path_to_keras_model})')
    parser.add_argument('--path-to-tags', type=Path, default=path_real_tags,
                        help=f'Place where tags are stored (default: {path_real_tags})')
    parser.add_argument('--indices-in-output', type=int, default=indices_test,
                        help=f'Number of images processed by doryta (default: {indices_test})')
    parser.add_argument('--outdir-doryta', type=Path, default=doryta_output,
                        help=f'Output folder of doryta execution (default: {doryta_output})')
    parser.add_argument('--model-type', default="fully",
                        help='Either "fully" or "lenet" (default: "fully")')
    parser.add_argument('--shift', type=int, default=None,
                        help='Doryta ID in which output spikes start')
    parser.add_argument('--dataset', default='mnist',
                        help='Either `mnist` or `fashion-mnist` (default: `mnist`)')
    args = parser.parse_args()

    if args.model_type == "fully":
        shift = 28*28 + 256 + 64
        which = ModelType.Fully
    elif args.model_type == "lenet":
        # shift = 38448 - 100
        shift = 8968 - 100
        which = ModelType.LeNet
    else:
        raise Exception(f'Model type "{args.model_type}" not recognized.')

    if args.shift is not None:
        shift = args.shift

    check_doryta_output_to_keras(args.path_to_keras_model, args.path_to_tags,
                                 slice(args.indices_in_output), args.outdir_doryta,
                                 shift, which=which, dataset=args.dataset)
