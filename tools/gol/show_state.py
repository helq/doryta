"""
Plots GoL from Doryta output file
"""

from __future__ import annotations

import numpy as np
import glob
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
    escaped_path = pathlib.Path(glob.escape(path))  # type: ignore
    stat_files = glob.glob(str(escaped_path / "*-spikes-gid=*.txt"))
    if not stat_files:
        print(f"No valid spike files have been found in path {path}", file=sys.stderr)
        exit(1)

    spikes = np.loadtxt(fileinput.input(stat_files))
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
    parser.add_argument('--save-as', type=pathlib.Path,
                        help='Path to save img or video (default: None)', default=None)
    args = parser.parse_args()

    imgs = extract_images_from_doryta_output(args.path, args.size)
    # np.save(f"{args.save_as}.raw.npy", imgs)  # Saving binary
    # imgs = np.load(f"{args.save_as}.raw.npy")

    print(f"There are {imgs.shape[0]} GoL steps stored in the file.")

    # Awesome trick to save image(s) from: https://stackoverflow.com/a/19696517
    dpi = 80  # Arbitrary value, allegedly it doesn't do anything
    height, width = imgs[0].shape
    zoom = 1.0
    width_in_pixels = max(height*16/9, width)
    # 650 is the width for the smallest grid
    if width_in_pixels < 650:
        zoom = 650 / width_in_pixels
    height, width = height * zoom / dpi, width * zoom / dpi
    fig = plt.figure(figsize=(width, height), dpi=dpi)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.axis('off')

    if args.iteration is not None:
        if not (0 <= args.iteration < imgs.shape[0]):
            print(f"Iteration {args.iteration} outside of bounds.",
                  file=sys.stderr)
            exit(1)

        ax.imshow(imgs[args.iteration], cmap='Greys')

        if args.save_as:
            fig.savefig(f"{args.save_as}.png")
        else:
            plt.show()
    else:
        imgplt = ax.imshow(imgs[0], cmap='Greys')
        anim = animation.FuncAnimation(
            plt.gcf(),
            lambda i: imgplt.set_data(imgs[i]),
            frames=imgs.shape[0],
            repeat=False,
            interval=args.speed  # milliseconds
        )
        if args.save_as:
            # extra_args = ["-preset", "ultrafast", "-qp", "0"]
            extra_args = ["-preset", "veryslow", "-qp", "0"]
            anim.save(f"{args.save_as}.m4v",
                      animation.FFMpegWriter(fps=1000/args.speed, codec='h264',
                                             extra_args=extra_args))
            # anim.save(f"{args.save_as}.gif", animation.ImageMagickWriter(fps=1000/args.speed))
        else:
            plt.show()
