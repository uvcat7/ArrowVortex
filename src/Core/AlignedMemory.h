#pragma once
#include <cstdlib>

template <typename T>
inline T* AlignedMalloc(size_t count) {
#ifndef _WIN32
    return static_cast<T*>(std::aligned_alloc(16, count * sizeof(T)));
#else
    return static_cast<T*>(_aligned_malloc(16, count * sizeof(T)));
#endif
}

inline void AlignedFree(void* ptr) {
    if (ptr) {
        std::free(ptr);
        ptr = nullptr;
    }
}
