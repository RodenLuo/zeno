# ZENO

[![CMake](https://github.com/zenustech/zeno/actions/workflows/cmake.yml/badge.svg)](https://github.com/zenustech/zeno/actions/workflows/cmake.yml) [![License](https://img.shields.io/badge/license-MPLv2-blue)](LICENSE) [![Version](https://img.shields.io/github/v/release/zenustech/zeno)](https://github.com/zenustech/zeno/releases)

Open-source node system framework, to change your algorithmic code into useful tools to create much more complicated simulations!

![rigid3.zsg](/images/rigid3.jpg "graphs/wip/rigid3.zsg")

ZENO is an OpenSource, Node based 3D system able to produce cinematic physics effects at High Efficiency, it was designed for large scale simulations and has been tested on complex setups.
Aside of its simulation Tools, ZENO provides necessary visualization nodes for users to import and run simulations if you feel that the current software you are using is too slow.


## Features

Integrated Toolbox, from volumetric geometry process tools (OpenVDB), to state-of-art, commercially robust, highly optimized physics solvers and visualization
nodes, and various VFX and simulation solutions based on our nodes (provided by .zsg file in `graphs/` folder).

## Gallery

![robot hit water](/images/crag_hit_water.gif)

![SuperSonic Flow](/images/shock.gif)


# End-user Installation

## Download binary release

Go to the [release page](https://github.com/zenustech/zeno/releases/), and click Assets -> download `zeno-linux-20xx.x.x.tar.gz`.
Then, extract this archive, and simply run `./launcher` (`launcher.exe` for Windows), then the node editor window will shows up if everything is working well.

## How to play

There are some example graphs in the `graphs/` folder, you may open them in the editor and have fun!
Hint: To run an animation for 100 frames, change the `1` on the top-left of node editor to `100`, then click `Execute`.
Also MMB to drag in the node editor, LMB click on sockets to create connections. MMB drag in the viewport to orbit camera, Shift+MMB to pan camera.

## Bug report

If you find the binary version didn't worked properly or some error message has been thrown on your machine, please let me know by opening an [issue](https://github.com/zenustech/zeno/issues) on GitHub, thanks for you support!


# Developer Build

ZENO is a CMake project, for detailed build instructions, please see [docs/build.md](/docs/build.md).


# Miscellaneous

## Write your own extension!

See https://github.com/zenustech/zeno_addon_wizard for an example on how to write custom nodes in ZENO.

## Contributors

Thank you to all the people who have already contributed to ZENO!

[![Contributors](https://contrib.rocks/image?repo=zenustech/zeno)](https://github.com/zenustech/zeno/graphs/contributors)

## License

ZENO is licensed under the Mozilla Public License Version 2.0, see [LICENSE](LICENSE) for more information.

## Contact us

You may contact us via WeChat:

* @zhxx1987: shinshinzhang

* @archibate: tanh233
