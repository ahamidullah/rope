#include <assert.h>
#include <stdarg.h>
#include <stddef.h> // size_t
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include "definitions.h"

typedef void *(*Thread_Procedure)(void *);

void application_entry();

#include "unicode_table.cpp"

// Platform specific source files.
#ifdef __linux__ 
#include <GL/gl.h>
#include <GL/glx.h>
#include "linux.cpp"

#elif _WIN32
#error "Windows version incomplete."

#elif __APPLE__
#error "Apple version incomplete."

#else
#error "Unsupported operating system."
#endif

#include "platform.h"

#include "file_offset.h"
#include "math.cpp"
/*
#include "memory.h"
#include "library.h"
#include "input.h"
*/

//#include "input.cpp"
#include "memory.cpp"
#include "library.cpp"
//#include "debug.cpp"
#include "sound.cpp"

