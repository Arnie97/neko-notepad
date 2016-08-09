# Hex Editor
This tool is developed to view the internal memory of HP 39 series.

## Features
- [x] Installs on a vanilla 39gs with no hardware hacks
- [x] Packed as an aplet, and provides a simple user interface
- [x] Well tested on my device
- [x] Free as in speech and as in beer

## Usage
Key                      | Function
-------------------------|-------------------------------
`[◄]`, `[►]`             | Move the cursor
`[▲]`, `[▼]`             | Change the digit above cursor
`[0]`-`[9]`, `[A]`-`[F]` | Input the RAM address directly
`[APLET]`                | Exit this program

## Prerequisites
You need the following packages before building clock tuner:

- GNU Make
- An toolchain for the `arm-elf` ABI
- `minstub39`, to combine ARM native code with SysRPL-based aplet skeleton
- `hplib`, which is included in HP-GCC v1.1
- `elf2hp`, also included in HP-GCC distributions

## Build
Clone the source code from GitHub, and `$ make believe`.

## Install
Use the official connectivity kit (conn3x) to transfer it into your calculator.
The 'HP 39/40' methods are much faster than the 'Disk drive' methods, but
won't help you install library dependencies, so you should transfer both
`hex_editor.apt` and `lib275.sys` to your calculator manually.

## Contributions
[Stars](https://github.com/Arnie97/clock-tuner/stargazers), [pull requests](https://github.com/Arnie97/clock-tuner/pulls) and [bug reports](https://github.com/Arnie97/clock-tuner/issues) are welcome.

## License
Hex Editor is distributed under the GPLv2 license.
See [`LICENSE`](LICENSE) for details.
