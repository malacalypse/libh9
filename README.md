# libh9
A C library for communicating with and modeling the remote behaviour of an Eventide H9 multi-effect unit.

More will be written on this as I have time to expand on it. 

## Summary

This library contains the `libh9.h` main header file (which is all you need to include to use the core h9 object and API). 

There are also some example projects which use this library in various useful ways - such as a commandline sysex parsing and validation tool, and a test harness which runs the unit testing framework.

The project is currently set up entirely in XCode. I will be porting it entirely to CMake at some point in the nearish future as XCode annoys me when I'm not building GUI apps.  

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
