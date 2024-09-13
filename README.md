# GDSiON

**GDSiON** is a software synthesizer library implemented as a GDExtension for the [Godot engine](https://godotengine.org/). It is based on [SiON](https://github.com/keim/SiON), a software synthesizer for Flash and Adobe AIR platforms, and just like its predecessor _GDSiON_ is self-sufficient and allows for easy dynamic sound generation. You can drive the process using a SiON-specific flavor of Music Macro Language (MML) or by making direct API calls.

The name of the synthesizer should be pronounced like the word "_scion_".

[![patreon-link](https://img.shields.io/badge/Patreon-orange?label=support%20the%20project&color=%23F2614B&style=for-the-badge)](https://patreon.com/YuriSizov)
[![discord-link](https://img.shields.io/badge/Discord-purple?label=get%20in%20touch&color=%235865F2&style=for-the-badge)](https://discord.gg/S657Y9KPF9)

## Download

This project is in the _beta_ phase. This means it's feature complete, but still requires some bug fixing and testing. Please make backups when working with prerelease libraries! [Bug reports](https://github.com/YuriSizov/gdsion/issues) are highly appreciated.

The project is compatible with **Godot 4.3**.

> [!NOTE]
> As _Godot 4.3_ is still being developed, there might be compatibility issues between _GDSiON_ and available builds of the engine. The project has been developed and tested with the [4.3-stable](https://godotengine.org/download/archive/4.3-stable/) release, so this is the minimum recommended version for now.

### Current release: 0.7-beta3

* **[Download for Linux](https://github.com/YuriSizov/gdsion/releases/download/0.7-beta3/libgdsion-linux.zip)**
* **[Download for macOS](https://github.com/YuriSizov/gdsion/releases/download/0.7-beta3/libgdsion-macos.zip)**
* **[Download for Windows](https://github.com/YuriSizov/gdsion/releases/download/0.7-beta3/libgdsion-windows.zip)**
* **[Download for Web](https://github.com/YuriSizov/gdsion/releases/download/0.7-beta3/libgdsion-web.zip)** (requires _4.3-beta1_ or later)
* **[Download for Android](https://github.com/YuriSizov/gdsion/releases/download/0.7-beta3/libgdsion-android.zip)**

_These archives contain both release and debug binaries._

If you need the most recent fixes, you can also download the _[latest unstable](https://github.com/YuriSizov/gdsion/releases/latest-unstable)_ version, built from the latest commit of the `main` branch.

## Setup

1. [Download GDSiON](#download) or [build the project yourself](#building).
2. Copy the `bin` folder from the archive (or your compilation results) to your project's root folder.
3. Make sure that you now have a `res://bin` folder, and that it contains `libgdsion.gdextension` and some other files starting with `libgdsion`.
4. Restart the editor, and start using the synth!

> [!TIP]
> You can type "SiON" in the search bar of the editor's built-in help to find available classes.

## Usage

First, you need to create a `SiONDriver` instance and add it to the scene tree. **Only one driver** can exist at the same time. (Some internal global objects are stateful and having multiple drivers can lead to conflicts.)

```gdscript
var driver := SiONDriver.create()
add_child(driver)
```

To confirm that everything works as intended, try playing a simple tune in the MML format, once the driver node is `ready`:

```gdscript
driver.play("t100 l8 [ ccggaag4 ffeeddc4 | [ggffeed4]2 ]2")
```

You can also play a melody by directly feeding the driver your notes, one by one or as a sequence.

_GDSiON_ is capable of emulating a variety of instruments and algorithms, which you can configure for your playback. You can use `SiONVoicePresetUtil` to generate presets for over 650 instrument voices.

**Check the example project for an interactive demo!**

* **[Download the example project source](https://github.com/YuriSizov/gdsion/releases/download/latest-unstable/example-project-source.zip)**

_The example project archive contains GDSiON binaries for all platforms. May require you to open the project twice to import everything correctly._

## Projects using GDSiON

Another way to learn more about _GDSiON_'s capabilities is to try one of the projects below:

* **[Bosca Ceoil Blue](https://github.com/YuriSizov/boscaceoil-blue)** — a beginner-friendly music making app.

## Building

_GDSiON_ is implemented on top of [godot-cpp](https://github.com/godotengine/godot-cpp), a C++ wrapper for GDExtension. It shares requirements with it and the engine itself.

The project uses the SCons build tool, and, once it's installed on your system, _GDSiON_ can be compiled as easily as running:

```shell
scons
```

There are several build options available, allowing you to target a specific platform or build configuration, enable development features and change optimization level. Please refer to [Godot documentation for developers](https://docs.godotengine.org/en/latest/contributing/development/compiling/index.html) for more details.

> [!NOTE]
> This project uses [Git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules). You need to use a git client to correctly check out everything needed for the build. The ZIP archive generated by GitHub **does not** contain submodules and is not enough to compile _GDSiON_.
> ```
> git clone git@github.com:YuriSizov/gdsion.git
> git submodule init
> ```

## How can I help the project?

First and foremost, using _GDSiON_ and reporting problems is already a great help! Please, don't hesitate to submit reports of any stability issues, incorrect behavior, or missing API access for some classes and functions.

If you have a functional suggestion, feel free to submit it as well! But please do be patient. Authors of this project are not very well versed in audio and music production. It may take a bit of time to understand and address your needs.

### Contributions and pull-requests

At this stage the project is considered feature complete. The main focus right now is on testing and improving the general state of it. You can help the efforts by creating unit tests, enhancing CI with additional checks and code style validation, writing documentation, etc. For code contributions, the biggest target is addressing memory management issues. There is also a number of TODOs and FIXMEs already present in the codebase which can be a good starting point for a quality PR!

If you want to work on a new feature, or perhaps have an idea for a better code architecture and design, please try to get in touch first. Such contributions are ultimately welcome, however the project is still going through its early days, so changes must be considered carefully.

There are also a few features that didn't make it into this initial release, namely MIDI support and Godot audio stream support for input and sampling. These are areas worth of exploration as a target for a future release of the library.

## License

This project is provided under an [MIT license](LICENSE). Original SiON software synthesizer library is provided under an [MIT license](https://github.com/keim/SiON/blob/1e6d6cd20bbc0379f5a81f607ac87a105163648f/LICENSE.md).

## Support

You can support the project financially by donating via [Patreon](https://www.patreon.com/YuriSizov)! Every dollar helps, so please consider donating even if it's a little. Thank you very much <3
