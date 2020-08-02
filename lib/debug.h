/*  debug.h
    This file is part of libh9, a library for remotely managing Eventide H9
    effects pedals.

    Copyright (C) 2020 Daniel Collins

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
 */

#ifndef debug_h
#define debug_h

#ifndef NDEBUG

#define DEBUG_ERROR 1
#define DEBUG_INFO  5
#define DEBUG_ANNOY 10

#ifdef DEBUG_LEVEL

#if (DEBUG_LEVEL >= DEBUG_ANNOY)
#define debug_annoy(...) printf(__VA_ARGS__)
#else
#define debug_annoy(...)
#endif

#if (DEBUG_LEVEL >= DEBUG_INFO)
#define debug_info(...) printf(__VA_ARGS__)
#else
#define debug_info(...)
#endif

#if (DEBUG_LEVEL >= DEBUG_ERROR)
#define debug_error(...) printf(__VA_ARGS__)
#else
#define debug_error(...)
#endif

#else  // DEBUG_LEVEL
#define debug_info(...)
#define debug_error(...)
#define debug_annoy(...)
#endif  // DEBUG_LEVEL

#else
#define debug_info(...)
#define debug_error(...)
#define debug_annoy(...)
#endif

#endif /* debug_h */
