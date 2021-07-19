# Building ioquake3 on Windows

The following instructions will guide you through building ioquake3 on Windows using either Cygwin or MSYS2

- [Building ioquake3 on Windows](#building-ioquake3-on-windows)
  - [Windows - cygwin](#windows---cygwin)
    - [Detailed Guide](#detailed-guide)
      - [1. Install Cygwin](#1-install-cygwin)
      - [2. Package selection](#2-package-selection)
      - [3. Install packages](#3-install-packages)
      - [4. Using Cygwin to check out and build ioquake3](#4-using-cygwin-to-check-out-and-build-ioquake3)
  - [Windows - msys2](#windows---msys2)
    - [64-Bit Binaries](#64-bit-binaries)
    - [32-Bit Binaries](#32-bit-binaries)

## Windows - cygwin

How to compile ioquake3 using MinGW in a Cygwin environment.

**NOTE: These instructions apply to the git version.**

1. Install Cygwin and required packages (see guide below).
2. Check out the source from [git://github.com/ioquake/ioq3.git](git://github.com/ioquake/ioq3.git) with

```sh
git clone git://github.com/ioquake/ioq3.git
```

3. Change to the ioq3 directory
4. Run `make`

### Detailed Guide

Detailed guide based on a [post by MAN-AT-ARMS](http://community.ioquake.org/t/how-to-build-ioquake3-using-cygwin/223).

#### 1. Install Cygwin

Download the Cygwin setup package from http://cygwin.com/install.html.

Choose either the 32-bit or 64-bit environment. 32-bit will work fine on both 32 and 64 bit versions of Windows. The setup program is also your Cygwin environment updater. If you have an existing Cygwin environment, the setup program will, by default, update your existing packages.

- Choose where you want to install Cygwin. The entire environment is self-contained in it's own folder, but you can also interact with files from outside the environment if you want to as well. The default install path is `C:\Cygwin`.
- Choose a mirror to download packages from, such as the [kernel.org](https://kernel.orgs) mirrors.
- Choose a "storage area" for your package downloads.

#### 2. Package selection

The next screen you see will be the package selections screen. In the upper left is a search box. This is where you will want to search for the necessary packages.

These are the package names you'll want to search for:

- `mingw64-i686-gcc-core` (For building 32bit binaries)
- `mingw64-i686-gcc-g++` (Also for 32bit... C++ support... not required for ioquake3, but useful for compiling other software)
- `mingw64-x86_64-gcc-core` (For building 64bit binaries)
- `mingw64-x86_64-gcc-g++` (For 64bit, same as above)
- `make`
- `bison`
- `git`

When you search for your packages you'll see category listings. These packages would all be under the **Devel** category.

To select a package, change the 'Skip' to the version of the package you want to install. The first click will be the newest version and subsequent clicks will allow you to choose older versions of the package. In our case here, you're probably good choosing the latest and greatest.

#### 3. Install packages

After you have selected your packages, just hit **Next** in the lower right. Cygwin will automatically add package dependencies. Hit next again to let the install run. Done.

The entire environment uses about 1GB of disk space (as opposed to about 6GB for a Visual Studio install).

#### 4. Using Cygwin to check out and build ioquake3

After the install has completed you should have a 'Cygwin Terminal' icon on your Desktop. This is the bash shell for Cygwin, so go ahead and run it.

At the command prompt type:

```sh
git clone git://github.com/ioquake/ioq3.git
```

This will download the ioquake3 master branch source into a new ioq3 folder.

Change to the new ioq3 folder that was created:

```sh
cd ioq3
```

You can build ioquake3 for 32 or 64 bit Windows by running one of the following

- `make ARCH=x86` (32 bit)`
- `make ARCH=x86_64` (64 bit)

After the build completes the output files will be in the 'build' folder. For the default Cygwin install the path it would be C:\Cygwin\home\Your_Username\ioq3\build\release-mingw32-arch.

---

## Windows - msys2

How to compile ioquake3 using MinGW in a MSYS2 environment.

### 64-Bit Binaries

To build 64-bit binaries, follow these instructions:

1. Install **msys2** from [https://msys2.github.io/](https://msys2.github.io/), following the instructions there. It doesn't matter which version you download, just get one appropriate for your OS.
2. Start **MSYS2 MinGW 64-bit** from the Start Menu.
3. Install **mingw-w64-x86_64**:

```sh
pacman -S mingw-w64-x86_64-gcc
```

4. Install **make**

```sh
pacman -S make
```

5. Grab latest ioq3 source code from GitHub. Use git, or just grab [https://github.com/ioquake/ioq3/archive/master.zip](https://github.com/ioquake/ioq3/archive/master.zip) and unzip it somewhere.
6. Change directory to where you put the source and run **make**:
   _Note that in msys2, your drives are linked as folders in the root directory: C:\ is /c/, D:\ is /d/, and so on._

```sh
cd /c/ioq3
make ARCH=x86_64
```

7. Find the executables and dlls in `build/release-mingw64-x86_64`.

### 32-Bit Binaries

To build 32-bit binaries, follow these instructions:

1. Install **msys2** from [https://msys2.github.io/](https://msys2.github.io/), following the instructions there.

2. Start **MSYS2 MinGW 32-bit** from the Start Menu.

3. Install **mingw-w64-i686-gcc**:

```sh
pacman -S mingw-w64-i686-gcc
```

4.  Install **make** from **msys**:

```sh
pacman -S make
```

5.  Grab latest ioq3 source code from GitHub. Use `git`, or just grab [https://github.com/ioquake/ioq3/archive/master.zip](https://github.com/ioquake/ioq3/archive/master.zip) and unzip it somewhere.

6.  Change directory to where you put the source and run make:
    _Note that in msys2, your drives are linked as folders in the root directory: C:\ is /c/, D:\ is /d/, and so on._

```sh
cd /c/ioq3
make ARCH=x86 WINDRES="windres -F pe-i386"
```

7. Find the executables and dlls in `build/release-mingw32-x86`.
