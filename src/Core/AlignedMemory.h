#pragma once
#include <SDL3/SDL_stdinc.h>

template <typename T>
inline T* AlignedMalloc(size_t count) {
    return static_cast<T*>(SDL_aligned_alloc(16, count * sizeof(T)));
}

inline void AlignedFree(void* ptr) {
    if (ptr) {
        SDL_aligned_free(ptr);
        ptr = nullptr;
    }
}
