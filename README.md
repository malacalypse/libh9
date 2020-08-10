# libh9
A C library for communicating with and modeling the remote behaviour of an Eventide H9 multi-effect unit.

More will be written on this as I have time to expand on it. 

## Summary

This library contains the `libh9.h` main header file (which is all you need to include to use the core h9 object and API). 

The examples dir is really not useful right now, that's a work in progress.

## Building / Testing

To build, follow the classic CMake pattern:

1. `mkdir build` (or choose a suitable directory name: Debug, Release, Test, etc.)
1. `cd build` (use the dirname you chose above)
1. `cmake ..` (for an explicitly debug or release build, `cmake -DCMAKE_BUILD_TYPE=Debug ..`, substitute Release for Debug as appropriate.)
1. `make` (if you want to make only a specific target, you can `make libh9` to build only the library, `make unittests` to build and run the tests, and `make coverage` to run the tests and generate a coverage report)

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
