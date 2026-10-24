#pragma once

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#undef max
#undef min
#undef ERROR
#else
#include "SDL3/SDL_opengl.h"
#include "SDL3/SDL_opengl_glext.h"
#endif
