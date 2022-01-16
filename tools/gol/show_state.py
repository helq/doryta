"""
Plots GoL from Doryta output file
"""

from __future__ import annotations

import numpy as np
from glob import glob
import fileinput
import argparse
import sys
import pathlib
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from typing import Any


def extract_images_from_doryta_output(
    path: pathlib.Path, width: int = 20
) -> np.ndarray[Any, Any]:
    stat_files = glob(str(path / "*-spikes-gid=*.txt"))
    if not stat_files:
        print(f"No valid spike files have been found in path {path}", file=sys.stderr)
        exit(1)

    spikes = np.loadtxt(fileinput.input(stat_files))  # type: ignore
    assert(len(spikes.shape) == 2)
    assert(spikes.shape[1] == 2)

    spikes = spikes[spikes[:, 0] < width*width, :].astype(int)
    steps = int(spikes[:, 1].max())

    imgs = np.zeros((width * width, steps))

    imgs[(spikes[:, 0], spikes[:, 1] - 1)] = 1

    return imgs.reshape((width, width, -1)).transpose((2, 0, 1))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=pathlib.Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--iteration', type=int,
                        help='Number of GoL time step. If not given, an animation will '
                        'show up instead', default=None)
    parser.add_argument('--speed', type=int,
                        help='Time between frames in milliseconds (default: 200)', default=200)
    parser.add_argument('--size', type=int,
                        help='Width/height of the grid (default: 20)', default=20)
    args = parser.parse_args()

    imgs = extract_images_from_doryta_output(args.path, args.size)

    print(f"There are {imgs.shape[0]} GoL steps stored in the file.")

    if args.iteration is not None:
        if not (0 <= args.iteration < imgs.shape[0]):
            print(f"Iteration {args.iteration} outside of bounds.",
                  file=sys.stderr)
            exit(1)

        imgplt = plt.imshow(imgs[args.iteration])
    else:
        imgplt = plt.imshow(imgs[0])
        anim = animation.FuncAnimation(
            plt.gcf(),
            lambda i: imgplt.set_data(imgs[i]),
            frames=imgs.shape[0],
            repeat=False,
            interval=args.speed  # milliseconds
        )
    plt.show()
