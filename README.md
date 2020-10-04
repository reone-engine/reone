# reone

[![Build Status](https://travis-ci.com/seedhartha/reone.svg?branch=master)](https://travis-ci.com/seedhartha/reone)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/21827/badge.svg)](https://scan.coverity.com/projects/seedhartha-reone)

reone is a free and open source game engine, capable of running Star Wars: Knights of the Old Republic and its sequel, The Sith Lords. We aim to provide an engine that is flexible and modern, and can be used to:

* Play KotOR and TSL from beginning to end
* Create mods that drastically change the gameplay or improve the visuals

Currently, the most fundamental engine subsystems are already implemented. Actual gameplay, however, is very limited. See project [gallery](https://github.com/seedhartha/reone/wiki/Gallery) and [roadmap](https://github.com/seedhartha/reone/wiki/Roadmap) in the Wiki.

## Similar Projects

reone is not the only project aiming to reimplement the Odyssey engine – there are at least four alternatives:

* [xoreos](https://github.com/xoreos/xoreos) – a more ambitious and a more established project, which I had contributed to and use alot as a reference. It is, however, not very active anymore, and I find it to be too complex and unoptimized for my taste.
* [KotOR.js](https://github.com/KobaltBlu/KotOR.js) – a fascinating JavaScript-based remake.
* [KotOR-Unity](https://github.com/rwc4301/KotOR-Unity) – a Unity-based reimplementation.
* The Northern Lights by [Lachlan O'Neill](https://github.com/lachjames) – another Unity-based reimplementation and a level editor.

## Installation

Those wishing to test reone, can either download a prebuilt executable from the [releases](https://github.com/seedhartha/reone/releases) page, or build it from source. Building it requires a modern C++ compiler and a set of free and open source libraries. See detailed instructions for your operating system [here](https://github.com/seedhartha/reone/wiki/Installation).

To run a game using reone, you must legally own it – we do not endorse piracy. Nowadays both games can be downloaded from Steam or GOG.com. Keep in mind that reone is tested against vanilla versions of the games – mod support is not guaranteed.

Install [Visual C++ Redistributable](https://aka.ms/vs/16/release/vc_redist.x64.exe) to run prebuilt Windows executables.

## Usage

Specify a game path to run the game, e.g. `reone --game C:\swkotor`

Controls:
* Use WASD keys to move around
* Click on doors to open them

## Configuration

reone can be configured from either a command line or a configuration file. Configuration file, named "reone.cfg", must be located in the current directory. See complete list of options [here](https://github.com/seedhartha/reone/wiki/Program-options).

## Contribution

reone is in the early stages of development at the moment, meaning that the codebase is not entirely stable. Nevertheless, the project is open to potential contributors. Take a look at the [roadmap](https://github.com/seedhartha/reone/wiki/Roadmap) and [development](https://github.com/seedhartha/reone/wiki/Development) pages in the Wiki if you want to contribute.
