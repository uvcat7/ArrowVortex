#pragma once

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#undef max
#undef min
#undef ERROR
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#elif defined(__linux__)
#include "SDL3/SDL_opengl.h"
#include "SDL3/SDL_opengl_glext.h"
#endif
