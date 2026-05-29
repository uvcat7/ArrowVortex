#pragma once
#include <cstdlib>

template <typename T>
inline T* AlignedMalloc(size_t count) {
    return static_cast<T*>(std::aligned_alloc(16, count * sizeof(T)));
}

inline void AlignedFree(void* ptr) {
    if (ptr) {
        std::free(ptr);
        ptr = nullptr;
    }
}
