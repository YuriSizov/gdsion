Built from commit [${COMMIT_HASH}](https://github.com/YuriSizov/gdsion/commits/${COMMIT_HASH}/). If you experience issues, [please report them](https://github.com/YuriSizov/gdsion/issues) as soon as you can.

## Downloads

* **[Download for Linux](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/libgdsion-linux.zip)**
* **[Download for macOS](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/libgdsion-macos.zip)**
* **[Download for Windows](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/libgdsion-windows.zip)**
* **[Download for Web](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/libgdsion-web.zip)** (requires _Godot 4.3-beta1_ or later)
* **[Download for Android](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/libgdsion-android.zip)**

_These archives contain GDSiON binaries for both debug and release exports._

## Installation

1. Extract the contents of the archive to your project's root folder.
2. Make sure that you now have a `bin` folder (i.e. `res://bin`), and that it contains `libgdsion.gdextension` and some other files starting with `libgdsion`.
3. Restart the editor.

You must download a build for each platform you plan on exporting to. The `libgdsion.gdextension` is the same in every download and can be safely overwritten when extracting multiple archives.

## Example project

The example project showcases one of many applications of the synthesizer library in an interactive way. It's the best way to start experimenting with GDSiON!

* [Download for Linux (x86_64)](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/example-project-linux-x86_64.zip)
* [Download for macOS (Universal)](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/example-project-macos-universal.zip)
* [Download for Windows (x86_64)](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/example-project-windows-x86_64.zip)
* [Download for Windows (x86_32)](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/example-project-windows-x86_32.zip)
* [Download project source](https://github.com/YuriSizov/gdsion/releases/download/${VERSION_TAG}/example-project-source.zip)

_The example project source archive contains GDSiON binaries for all platforms. It may be required for you to open the project twice to import everything correctly._
