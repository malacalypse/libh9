# libh9
A C library for communicating with and modeling the remote behaviour of an Eventide H9 multi-effect unit.

More will be written on this as I have time to expand on it. 

## Summary

This library contains the `h9.h` main header file (which is all you need to include to use the core h9 object and API). 

There are also some example projects which use this library in various useful ways - such as a commandline sysex parsing and validation tool, and a test harness which runs the unit testing framework.

The project is currently set up entirely in XCode. I will be porting it entirely to CMake at some point in the nearish future as XCode annoys me when I'm not building GUI apps.  
