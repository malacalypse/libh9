# libh9
A C library for communicating with and modeling the remote behaviour of an Eventide H9 multi-effect unit. For an example of this library in use, check out my (rudimentary) Max/MSP external [H9 External](https://github.com/malacalypse/h9_external).

## Summary

To use libh9 in your project, include the header:
```C
#include "libh9.h"
```
If you are using CMake, you can take advantage of the build toolchain directly. Add something like this to your project's CMakeLists.txt:
```CMake
add_subdirectory(lib/libh9)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libh9/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE libh9)
```

## Building / Testing

To build libh9 directly (produces a .a file) or to build and run the test suite, follow the classic CMake pattern:

1. `mkdir build` (or choose a suitable directory name: Debug, Release, Test, etc.)
1. `cd build` (use the dirname you chose above)
1. `cmake ..` (for an explicitly debug or release build, `cmake -DCMAKE_BUILD_TYPE=Debug ..`, substitute Release for Debug as appropriate.)
1. `make` (if you want to make only a specific target, you can `make libh9` to build only the library, `make unittests` to build and run the tests, and `make coverage` to run the tests and generate a coverage report)

Builds are tested on MacOS. I do not provide support for using it on Windows.

## License

The full text of the license should be found in LICENSE.txt, included as part of this repository.

This library and all accompanying code, examples, information and documentation is
Copyright (C) 2019-2020 Daniel Collins

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
