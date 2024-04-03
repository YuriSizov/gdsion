# GDSiON

**GDSiON** is a software synthesizer library implemented as a GDExtension for the [Godot engine](https://godotengine.org/). It is originally based on [SiON](https://github.com/keim/SiON), a software synthesizer for the Flash and Adobe AIR platform, and just like its predecessor _GDSiON_ is self-sufficient and allows for easy dynamic sound generation. You can drive the process using Music Macro Language (MML) notation or direct API calls.

The name of the synthesizer should be pronounced like the word "_scion_".

## Setup

This project is still in development, so pre-compiled binaries are not yet available. You need to be able to [build the project yourself](#building) for the time being.

1. Check out and build the project.
2. Copy the build artifacts to your project's `res://bin/` folder, alongside the `*.gdextension` file.
3. Restart your project, and start using the synth! _Tip: You can type "SiON" in the editor's built-in help search to find available classes._

## Usage

First, you need to create a `SiONDriver` instance and add it to the scene tree. (Note, that only one driver can exist at the same time, as there are potential conflicts in some global objects.)

```gdscript
var driver := SiONDriver.new()
add_child(driver)
```

To confirm that everything works as intended, try playing a simple tune in the MML format:

```gdscript
driver.play("t100 l8 [ ccggaag4 ffeeddc4 | [ggffeed4]2 ]2")
```

You can also play a melody by directly feeding the driver your notes, one by one or as a sequence. GDSiON is capable of emulating a variety of instruments and algorithms, which you can configure for your playback.

**Check the [example project](/example) for an interactive demo!**

## Building

_GDSiON_ is implemented on top of [godot-cpp](https://github.com/godotengine/godot-cpp), a C++ wrapper for GDExtension. It shares requirements with it and the engine itself.

The project uses the SCons build tool, and, once it's installed on your system, it can be compiled as easily as running:

```shell
scons
```

There are several build options available, allowing you to target a specific platform or build configuration, enable development features and change optimization level. Please refer to [Godot documentation for developers](https://docs.godotengine.org/en/latest/contributing/development/compiling/index.html) for more details.

## How can I help?

You can help, first and foremost, by testing _GDSiON_ and reporting any stability issues, incorrect behavior, and missing API.

At the current stage the project is still considered to be incomplete, and efforts are made to port the remaining classes and systems to the new platform. As a developer and a potential contributor you can assist in these efforts. Please try to get in touch first if you want to work on porting any of the missing parts. Existing FIXMEs and TODOs are open for everyone.

We would also appreciate assistance with setting up the CI, introducing tests, code style enforcement, writing documentation, etc. Code style, code quality, and architecture/design suggestions are welcomed as well.

If you have a functional suggestion, feel free to submit it! But please do be patient. Authors of this project are not very well versed in audio production and currently aim to get what already exists in the original SiON project working and available in this extension. It may take a bit of time to understand and address your needs.

## License

This project is provided under an [MIT license](LICENSE). Original SiON software synthesizer library is provided under an [MIT license](https://github.com/keim/SiON/blob/1e6d6cd20bbc0379f5a81f607ac87a105163648f/LICENSE.md).

## Support

You can support the project financially by donating via [Patreon](https://www.patreon.com/YuriSizov)! Every dollar helps, so please consider donating even if it's a little. Thank you very much <3
