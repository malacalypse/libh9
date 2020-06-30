//
//  debug.h
//  h9
//
//  Created by Studio DC on 2020-06-30.
//

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

#else // DEBUG_LEVEL
#define debug_info(...)
#define debug_error(...)
#define debug_annoy(...)
#endif // DEBUG_LEVEL

#else
#define debug_info(...)
#define debug_error(...)
#define debug_annoy(...)
#endif

#endif /* debug_h */
