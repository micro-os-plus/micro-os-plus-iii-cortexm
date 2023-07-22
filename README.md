[![GitHub package.json version](https://img.shields.io/github/package-json/v/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/blob/xpack/package.json)
[![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/tags/)
[![license](https://img.shields.io/github/license/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/blob/xpack/LICENSE)

# A source code library with the µOS++ Cortex-M port (an xpm/npm package)

This project provides support for running µOS++ on Cortex-M devices.

The open-source project is hosted on GitHub as
[micro-os-plus/micro-os-plus-iii-cortexm](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm).

## Maintainer info

This page is addressed to developers who plan to include this source
library into their own projects.

For maintainer info, please see the
[README-MAINTAINER](README-MAINTAINER.md) file.

## Install

As a source code library, this project can be integrated into another project
in the traditional way,
by either copying the relevant files into the target project, or by linking
the entire project as a Git submodule.

However, the workflow can be further automated and the most convenient way is
to **add it as a dependency** to the project via **xpm**.

### Install with xpm/npm

Along with the source files, this project also includes a
`package.json` file with the metadata that allows it to be identified as an
**xpm/npm** package that can be installed automatically as a dependency.

#### Prerequisites

A recent [xpm](https://xpack.github.io/xpm/),
which is a portable [Node.js](https://nodejs.org/) command line application
that complements [npm](https://docs.npmjs.com)
with several extra features specific to
**C/C++ projects**.

It is recommended to install/update to the latest version with:

```sh
npm install --global xpm@latest
```

For details please follow the instructions in the
[xPack install](https://xpack.github.io/install/) page.

Warning: Be sure **xpm** is not installed with administrative/root rights.

#### xpm

This source code library can be installed as a sub-folder below `xpacks` with:

```sh
cd my-project
xpm init # Add a package.json if not already present

xpm install github:micro-os-plus/micro-os-plus-iii-cortexm#v1.0.0 --save-dev --copy

ls -l xpacks/@micro-os-plus/micro-os-plus-iii-cortexm
```

Note: On Windows use `dir` and back-slashes.

#### npm

The package can also be installed with [npm](https://docs.npmjs.com)
or related, but the content will end in the `node_modules` folder,
along with JavaScript code;
therefore it is recommended to install C/C++ packages via **xpm**.

### Add as a Git submodule

If, for any reason, xpm/npm are not available, it is always possible
to manually copy the relevant files into the target
project. However, this will need extra maintenance to keep the
project up to date.

A more convenient
solution is to link the entire project as a **Git submodule**,
for example below an `xpacks` folder:

```sh
cd my-project
git init # Unless already a Git project
mkdir -p xpacks

git submodule add https://github.com/micro-os-plus/micro-os-plus-iii-cortexm.git \
  xpacks/@micro-os-plus/micro-os-plus-iii-cortexm
```

## Branches

Apart from the unused `master` branch, there are two active branches:

- `xpack`, with the latest stable version (default)
- `xpack-develop`, with the current development version

All development is done in the `xpack-develop` branch, and contributions via
Pull Requests should be directed to this branch.

When new releases are published, the `xpack-develop` branch is merged
into `xpack`.

## Developer info

### Overview

The µOS++ IIIe source code is split between a portable part
and platform specific code (like Cortex-M, this project).
Applications must include both.

### Status

The **micro-os-plus-iii-cortexm** source library is fully functional,
but starting with mid 2023 it was marked as end-of-life and
is not recommended for new designs.

A new package will be available
as part of µOS++ IVe.

### Build & integration info

The project is written in C/C++ and it is expected
to be used in C and C++ projects.

The source code was compiled with GCC 11/12, clang 12/13/14/15
arm-none-eabi-gcc 12, and should be warning free.

To ease the integration of this package into user projects, there
are already made CMake configuration files (see below).

For other build systems, consider the following details:

#### Include folders

The following folders should be passed to the compiler during the build:

- `include`

#### Source files

The source files to be added to the build are:

- `src/rtos/os-core.cpp`

#### Preprocessor definitions

- none

#### Compiler options

- `-std=c++20` or higher for C++ sources
- `-std=c11` for C sources

#### Dependencies

- none

#### CMake

To integrate the **micro-os-plus-iii-cortexm** source library
into a CMake application,
add this folder to the build:

```cmake
add_subdirectory("xpacks/@micro-os-plus/micro-os-plus-iii-cortexm")`
```

The result is an interface library that can be added as an application
dependency with:

```cmake
target_link_libraries(your-target PRIVATE

  micro-os-plus::iii-cortexm
)
```

### Examples

- none

### Known problems

- none

### Limitations

- none

### Tests

- none

## License

Unless otherwise stated, the content is released under the terms of the
[MIT License](https://opensource.org/licenses/mit/),
with all rights reserved to
[Liviu Ionescu](https://github.com/ilg-ul).
