![GitHub package.json version](https://img.shields.io/github/package-json/v/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/blob/xpack/package.json)
[![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/tags/)
[[![license](https://img.shields.io/github/license/micro-os-plus/micro-os-plus-iii-cortexm)](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/blob/xpack/LICENSE)

# A source code library with the µOS++ Cortex-M port

This project provides support for running µOS++ on Cortex-M devices.

The project is hosted on GitHub as
[micro-os-plus/micro-os-plus-iii-cortexm](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm).

## Maintainer info

This page is addressed to developers who plan to include this source
library into their own projects.

For maintainer info, please see the
[README-MAINTAINER](README-MAINTAINER.md) file.

## Install

As a source library xPack, the easiest way to add it to a project is via
**xpm**, but it can also be used as any Git project, for example as a submodule.

### Prerequisites

A recent [xpm](https://xpack.github.io/xpm/),
which is a portable [Node.js](https://nodejs.org/) command line application.

It is recommended to update to the latest version with:

```sh
npm install --global xpm@latest
```

For details please follow the instructions in the
[xPack install](https://xpack.github.io/install/) page.

### xpm

This package is available as
`@micro-os-plus/micro-os-plus-iii-cortexm` from GitHub.

```sh
cd my-project
xpm init # Unless a package.json is already present

xpm install github:micro-os-plus/micro-os-plus-iii-cortexm

ls -l xpacks/@micro-os-plus/micro-os-plus-iii-cortexm
```

### Git submodule

If, for any reason, **xpm** is not available, the next recommended
solution is to link it as a Git submodule below an `xpacks` folder.

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

TBD

### Status

The **micro-os-plus-iii-cortexm** source library is fully functional,
but starting with mid 2023 it was marked as end-of-life and
is not recommended for new designs.

### Build & integration info

The project is written in C/C++ and it is expected
to be used in C and C++ projects.

The source code was compiled with GCC 11, clang 12, clang 13
arm-none-eabi-gcc 11, and should be warning free.

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

  micro-os-plus::micro-os-plus-iii-cortexm
)
```

### Examples

- none

### Known problems

- none

#### Limitations

- none

### Tests

- none

## Change log - incompatible changes

According to [semver](https://semver.org) rules:

> Major version X (X.y.z | X > 0) MUST be incremented if any
backwards incompatible changes are introduced to the public API.

The incompatible changes, in reverse chronological order,
are:

- v1.x: the original code.

## License

The original content is released under the
[MIT License](https://opensource.org/licenses/MIT/),
with all rights reserved to
[Liviu Ionescu](https://github.com/ilg-ul).
