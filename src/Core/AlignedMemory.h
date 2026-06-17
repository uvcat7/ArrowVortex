#pragma once

template <typename T>
inline T* AlignedMalloc(size_t count) {
    return static_cast<T*>(_aligned_malloc(count * sizeof(T), 16));
}

inline void AlignedFree(void* ptr) {
    if (ptr) {
        _aligned_free(ptr);
        ptr = nullptr;
    }
}
