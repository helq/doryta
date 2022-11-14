from __future__ import annotations

import numpy as np
import glob
import fileinput
import argparse
import sys
import pathlib
from math import ceil

from typing import Any


def extract_spikes_from_doryta_output(
    path: pathlib.Path,
    from_: int,
    to: int,
    scale: float = 1.0
) -> tuple[np.ndarray[Any, Any], np.ndarray[Any, Any]]:
    escaped_path = pathlib.Path(glob.escape(path))  # type: ignore
    stat_files = glob.glob(str(escaped_path / "spikes-gid=*.txt"))
    if not stat_files:
        print(f"No valid spike files have been found in path {path}", file=sys.stderr)
        exit(1)

    spikes = np.loadtxt(fileinput.input(stat_files))

    # checking data shape
    if spikes.size == 0:
        spikes = spikes.reshape((0, 2))
    assert len(spikes.shape) == 2
    assert spikes.shape[1] == 2

    # Scaling spike inputs
    spikes[:, 1] *= scale
    # only spikes from the first `width*width` neurons are taken into account
    spikes = spikes[(from_ <= spikes[:, 0]) & (spikes[:, 0] <= to), :]
    times = np.unique(spikes[:, 1])

    outputs = np.zeros((times.shape[0], to - from_ + 1), dtype='int')
    steps = times.reshape((1, -1)) - spikes[:, 1].reshape((-1, 1))
    indices = np.argwhere(steps == 0)
    outputs[indices[:, 1], spikes[:, 0].astype(int) - from_] = 1

    return outputs, times


def check_complement(x: np.ndarray[Any, Any]) -> bool:
    assert len(x.shape) == 1
    assert x.shape[0] % 2 == 0
    n_half = x.shape[0] // 2

    return bool(np.all(x[:n_half] != x[n_half:]))


def pretty_bits(x: np.ndarray[Any, Any]) -> str:
    assert len(x.shape) == 1
    num_bits = x.shape[0]
    packs = np.packbits(x[::-1], bitorder='little')[::-1]

    int_bits = '[' + ' '.join(str(b) for b in packs) + ']'

    num_bits_first = 8 if num_bits % 8 == 0 else num_bits % 8
    raw_bits = f"{{:0{num_bits_first}b}} ".format(packs[0])
    raw_bits += ''.join(f"{b:08b} " for b in packs[1:])

    num_nibs_first = ceil(num_bits_first / 4)
    hex_bits = f"{{:0{num_nibs_first}X}}".format(packs[0])
    hex_bits += ''.join(f'{b:02X}' for b in packs[1:])

    ascii_bits = ''.join([chr(by) if chr(by).isascii() else '?' for by in packs])

    return f'{raw_bits}\t{int_bits}\t0x{hex_bits}\t{ascii(ascii_bits)}'


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', type=pathlib.Path, help='Output directory of doryta execution',
                        required=True)
    parser.add_argument('--from', type=int,
                        help='Output starting point (default: 17)', default=17, dest='from_')
    parser.add_argument('--to', type=int,
                        help='Output ending point (default: 44)', default=44)
    parser.add_argument('--scale', type=float,
                        help="Scale the timing (default: 1.0)", default=1.0)
    parser.add_argument('--complement', action='store_true',
                        help="Check that half of the output represents ones and the other "
                        "half zeros")
    parser.add_argument('--little', action='store_true',
                        help="Interprets the first bit as the most significant bit")
    args = parser.parse_args()

    outputs, times = extract_spikes_from_doryta_output(args.path, args.from_, args.to, args.scale)

    order = slice(None) if args.little else slice(None, None, -1)
    print('time\tbits\tints\thex\tascii')
    if args.complement:
        n_half = outputs.shape[1] // 2
        for t, out in zip(times, outputs):
            print(f"{t:<8}", end="\t")
            if not check_complement(out):
                print("(faulty output)", pretty_bits(out[order]))
            else:
                print(pretty_bits(out[:n_half][order]))
    else:
        for t, out in zip(times, outputs):
            print(t, end="\t")
            print(pretty_bits(out[order]))
