# How to Build

This wiki page provides detailed instructions for building OrcaSlicer from source on different operating systems, including Windows, macOS, and Linux.  
It includes tool requirements, setup commands, and build steps for each platform.

Whether you're a contributor or just want a custom build, this guide will help you compile OrcaSlicer successfully.

- [Windows 64-bit](#windows-64-bit)
  - [Windows Tools Required](#windows-tools-required)
  - [Windows Instructions](#windows-instructions)
- [MacOS 64-bit](#macos-64-bit)
  - [MacOS Tools Required](#macos-tools-required)
  - [MacOS Instructions](#macos-instructions)
  - [Debugging in Xcode](#debugging-in-xcode)
- [Linux](#linux)
  - [Using Docker](#using-docker)
    - [Docker Dependencies](#docker-dependencies)
    - [Docker Instructions](#docker-instructions)
  - [Troubleshooting](#troubleshooting)
  - [Linux Build](#linux-build)
    - [Dependencies](#dependencies)
      - [Common dependencies across distributions](#common-dependencies-across-distributions)
      - [Additional dependencies for specific distributions](#additional-dependencies-for-specific-distributions)
    - [Linux Instructions](#linux-instructions)
- [Portable User Configuration](#portable-user-configuration)
  - [Example folder structure](#example-folder-structure)

## Windows 64-bit

How to building with Visual Studio 2022 on Windows 64-bit.

### Windows Tools Required

- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) or Visual Studio 2019
  ```shell
  winget install --id=Microsoft.VisualStudio.2022.Professional -e
  ```
- [CMake (version 3.31)](https://cmake.org/) — **⚠️ version 3.31.x is mandatory**
  ```shell
  winget install --id=Kitware.CMake -v "3.31.6" -e
  ```
- [Strawberry Perl](https://strawberryperl.com/)
  ```shell
  winget install --id=StrawberryPerl.StrawberryPerl -e
  ```
- [Git](https://git-scm.com/)
  ```shell
  winget install --id=Git.Git -e
  ```
- [git-lfs](https://git-lfs.com/)
  ```shell
  winget install --id=GitHub.GitLFS -e
  ```

> [!TIP]
> GitHub Desktop (optional): A GUI for Git and Git LFS, which already includes both tools.
> ```shell
> winget install --id=GitHub.GitHubDesktop -e
> ```

### Windows Instructions

1. Clone the repository:
   - If using GitHub Desktop clone the repository from the GUI.
   - If using the command line:
     1. Clone the repository:
     ```shell
     git clone https://github.com/SoftFever/OrcaSlicer
     ```
     2. Run lfs to download tools on Windows:
     ```shell
     git lfs pull
     ```
2. Open the appropriate command prompt:
   - For Visual Studio 2019:  
     Open **x64 Native Tools Command Prompt for VS 2019** and run:
     ```shell
     build_release.bat
     ```
   - For Visual Studio 2022:  
     Open **x64 Native Tools Command Prompt for VS 2022** and run:
     ```shell
     build_release_vs2022.bat
     ```

> [!NOTE]
> If you encounter issues, you can try to uninstall ZLIB from your Vcpkg library.

3. If successful, you will find the VS 2022 solution file in:
   ```shell
   build\OrcaSlicer.sln
   ```

> [!IMPORTANT]
> Make sure that CMake version 3.31.x is actually being used. Run `cmake --version` and verify it returns a **3.31.x** version.
> If you see an older version (e.g. 3.29), it's likely due to another copy in your system's PATH (e.g. from Strawberry Perl).
> You can run where cmake to check the active paths and rearrange your **System Environment Variables** > PATH, ensuring the correct CMake (e.g. C:\Program Files\CMake\bin) appears before others like C:\Strawberry\c\bin.

> [!NOTE]
> If the build fails, try deleting the `build/` and `deps/build/` directories to clear any cached build data. Rebuilding after a clean-up is usually sufficient to resolve most issues.

## MacOS 64-bit

How to building with Xcode on MacOS 64-bit.

### MacOS Tools Required

- Xcode
- CMake (version 3.31.x is mandatory)
- Git
- gettext
- libtool
- automake
- autoconf
- texinfo

> [!TIP]
> You can install most of them by running:
> ```shell
> brew install gettext libtool automake autoconf texinfo
> ```

Homebrew currently only offers the latest version of CMake (e.g. **4.X**), which is not compatible. To install the required version **3.31.X**, follow these steps:

1. Download CMake **3.31.7** from: [https://cmake.org/download/](https://cmake.org/download/)
2. Install the application (drag it to `/Applications`).
3. Add the following line to your shell configuration file (`~/.zshrc` or `~/.bash_profile`):

```sh
export PATH="/Applications/CMake.app/Contents/bin:$PATH"
```

4. Restart the terminal and check the version:

```sh
cmake --version
```

5. Make sure it reports a **3.31.x** version.

> [!IMPORTANT]
> If you've recently upgraded Xcode, be sure to open Xcode at least once and install the required macOS build support.

### MacOS Instructions

1. Clone the repository:
   ```shell
   git clone https://github.com/SoftFever/OrcaSlicer
   cd OrcaSlicer
   ```
2. Build the application:
   ```shell
   ./build_release_macos.sh
   ```
3. Open the application:
   ```shell
   open build/arm64/OrcaSlicer/OrcaSlicer.app
   ```

### Debugging in Xcode

To build and debug directly in Xcode:

1. Open the Xcode project:
   ```shell
   open build/arm64/OrcaSlicer.xcodeproj
   ```
2. In the menu bar:
   - **Product > Scheme > OrcaSlicer**
   - **Product > Scheme > Edit Scheme...**
     - Under **Run > Info**, set **Build Configuration** to `RelWithDebInfo`
     - Under **Run > Options**, uncheck **Allow debugging when browsing versions**
   - **Product > Run**

## Linux

Linux distributions are available in two formats: [using Docker](#using-docker) (recommended) or [building directly](#linux-build) on your system.

### Using Docker

How to build and run OrcaSlicer using Docker.

#### Docker Dependencies

- Docker
- Git

#### Docker Instructions

```shell
git clone https://github.com/SoftFever/OrcaSlicer && cd OrcaSlicer && ./DockerBuild.sh && ./DockerRun.sh
```

### Troubleshooting

The `DockerRun.sh` script includes several commented-out options that can help resolve common issues. Here's a breakdown of what they do:

- `xhost +local:docker`: If you encounter an "Authorization required, but no authorization protocol specified" error, run this command in your terminal before executing `DockerRun.sh`. This grants Docker containers permission to interact with your X display server.
- `-h $HOSTNAME`: Forces the container's hostname to match your workstation's hostname. This can be useful in certain network configurations.
- `-v /tmp/.X11-unix:/tmp/.X11-unix`: Helps resolve problems with the X display by mounting the X11 Unix socket into the container.
- `--net=host`: Uses the host's network stack, which is beneficial for printer Wi-Fi connectivity and D-Bus communication.
- `--ipc host`: Addresses potential permission issues with X installations that prevent communication with shared memory sockets.
- `-u $USER`: Runs the container as your workstation's username, helping to maintain consistent file permissions.
- `-v $HOME:/home/$USER`: Mounts your home directory into the container, allowing you to easily load and save files.
- `-e DISPLAY=$DISPLAY`: Passes your X display number to the container, enabling the graphical interface.
- `--privileged=true`: Grants the container elevated privileges, which may be necessary for libGL and D-Bus functionalities.
- `-ti`: Attaches a TTY to the container, enabling command-line interaction with OrcaSlicer.
- `--rm`: Automatically removes the container once it exits, keeping your system clean.
- `orcaslicer $*`: Passes any additional parameters from the `DockerRun.sh` script directly to the OrcaSlicer executable within the container.

By uncommenting and using these options as needed, you can often resolve issues related to display authorization, networking, and file permissions.

### Linux Build

How to build OrcaSlicer on Linux.

#### Dependencies

The build system supports multiple Linux distributions including Ubuntu/Debian and Arch Linux. All required dependencies will be installed automatically by the provided shell script where possible, however you may need to manually install some dependencies.

> [!NOTE]
> Fedora and other distributions are not currently supported, but you can try building manually by installing the required dependencies listed below.

##### Common dependencies across distributions

- autoconf / automake
- cmake
- curl / libcurl4-openssl-dev
- dbus-devel / libdbus-1-dev
- eglexternalplatform-dev / eglexternalplatform-devel
- extra-cmake-modules
- file
- gettext
- git
- glew-devel / libglew-dev
- gstreamer-devel / libgstreamerd-3-dev
- gtk3-devel / libgtk-3-dev
- libmspack-dev / libmspack-devel
- libsecret-devel / libsecret-1-dev
- libspnav-dev / libspnav-devel
- libssl-dev / openssl-devel
- libtool
- libudev-dev
- mesa-libGLU-devel
- ninja-build
- texinfo
- webkit2gtk-devel / libwebkit2gtk-4.0-dev or libwebkit2gtk-4.1-dev
- wget

##### Additional dependencies for specific distributions

- **Ubuntu 22.x/23.x**: libfuse-dev, m4
- **Arch Linux**: mesa, wayland-protocols

#### Linux Instructions

1. **Install system dependencies:**
   ```shell
   ./build_linux.sh -u
   ```

2. **Build dependencies:**
   ```shell
   ./build_linux.sh -d
   ```

3. **Build OrcaSlicer:**
   ```shell
   ./build_linux.sh -s
   ```

4. **Build AppImage (optional):**
   ```shell
   ./build_linux.sh -i
   ```

5. **All-in-one build (recommended):**
   ```shell
   ./build_linux.sh -dsi
   ```

**Additional build options:**

- `-b`: Build in debug mode
- `-c`: Force a clean build
- `-C`: Enable ANSI-colored compile output (GNU/Clang only)
- `-j N`: Limit builds to N cores (useful for low-memory systems)
- `-1`: Limit builds to one core
- `-l`: Use Clang instead of GCC
- `-p`: Disable precompiled headers (boost ccache hit rate)
- `-r`: Skip RAM and disk checks (for low-memory systems)

> [!NOTE]
> The build script automatically detects your Linux distribution and uses the appropriate package manager (apt, pacman) to install dependencies.

> [!TIP]
> For first-time builds, use `./build_linux.sh -u` to install dependencies, then `./build_linux.sh -dsi` to build everything.

> [!WARNING]
> If you encounter memory issues during compilation, use `-j 1` or `-1` to limit parallel compilation, or `-r` to skip memory checks.

---

## Portable User Configuration

If you want OrcaSlicer to use a custom user configuration folder (e.g., for a portable installation), you can simply place a folder named `data_dir` next to the OrcaSlicer executable. OrcaSlicer will automatically use this folder as its configuration directory.

This allows for multiple self-contained installations with separate user data.

> [!TIP]
> This feature is especially useful if you want to run OrcaSlicer from a USB stick or keep different profiles isolated.

### Example folder structure

```shell
OrcaSlicer.exe
data_dir/
```

You don’t need to recompile or modify any settings — this works out of the box as long as `data_dir` exists in the same folder as the executable.
