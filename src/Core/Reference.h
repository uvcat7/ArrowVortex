#pragma once

#include <Core/Core.h>

#include <stdlib.h>
#include <new>

namespace Vortex {

// ================================================================================================

template <typename T>
class Reference {
   public:
    ~Reference();
    Reference();
    Reference(const Reference& v);
    Reference& operator=(const Reference& v);

    /// Dereferences the current reference and deletes it if count reaches zero.
    void destroy();

    /// Destroys the current reference, and creates a new reference.
    T* create();

    /// Destroys the current reference, and creates a new reference to a copy of
    /// value.
    void create(const T& value);

    /// Destroys the current reference, and references the value of another
    /// reference.
    void copy(const Reference<T>& other);

    /// Returns true if the current value is null.
    inline bool null() const { return !reference_ptr_; }

    /// Returns the number of references to the current value.
    inline int count() const {
        return reference_ptr_ ? *((int*)reference_ptr_ - 1) : 0;
    }

    /// Returns a pointer to the current value.
    inline operator T*() { return reference_ptr_; }

    /// Returns a const pointer to the current value.
    inline operator T*() const { return reference_ptr_; }

    /// Returns a pointer to the current value.
    inline T* operator->() { return reference_ptr_; }

    /// Returns a const pointer to the current value.
    inline const T* operator->() const { return reference_ptr_; }

    /// Returns a pointer to the current value.
    inline T* get() { return reference_ptr_; }

    /// Returns a const pointer to the current value.
    inline const T* get() const { return reference_ptr_; }

   private:
    void InitReference();
    T* reference_ptr_;
};

// ================================================================================================
// Anything below this line is used internally and is not part of the API.

template <typename T>
Reference<T>::~Reference() {
    destroy();
}

template <typename T>
Reference<T>::Reference() : reference_ptr_(nullptr) {}

template <typename T>
Reference<T>::Reference(const Reference& r) : reference_ptr_(nullptr) {
    copy(r);
}

template <typename T>
Reference<T>& Reference<T>::operator=(const Reference& r) {
    copy(r);
    return *this;
}

template <typename T>
T* Reference<T>::create() {
    InitReference();
    new (reference_ptr_) T();
    return reference_ptr_;
}

template <typename T>
void Reference<T>::create(const T& v) {
    InitReference();
    new (reference_ptr_) T(v);
}

template <typename T>
void Reference<T>::destroy() {
    if (reference_ptr_) {
        int* ref = (int*)reference_ptr_ - 1;
        if (--*ref == 0) {
            reference_ptr_->~T();
            delete ref;
        }
        reference_ptr_ = nullptr;
    }
}

template <typename T>
void Reference<T>::copy(const Reference& o) {
    if (reference_ptr_ != o.reference_ptr_) {
        destroy();
        reference_ptr_ = o.reference_ptr_;
        ++*((int*)reference_ptr_ - 1);
    }
}

template <typename T>
void Reference<T>::InitReference() {
    destroy();
    int* ref = (int*)malloc(sizeof(int) + sizeof(T));
    *ref = 1;
    reference_ptr_ = (T*)(ref + 1);
}

};  // namespace Vortex
