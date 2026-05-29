#pragma once

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#undef ERROR
#elifdef __linux__
#include "SDL3/SDL_opengl.h"
#include "SDL3/SDL_opengl_glext.h"
#endif
