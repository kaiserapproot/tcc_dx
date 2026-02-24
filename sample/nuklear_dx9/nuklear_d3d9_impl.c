#define NK_D3D9_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include <assert.h>
#ifndef NK_ASSERT
#define NK_ASSERT(expr) assert(expr)
#endif
#include "nuklear_d3d9.h"
