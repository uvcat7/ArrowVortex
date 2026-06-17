#pragma once

#include <Core/Vector.h>

namespace Vortex {

struct Vec {
// Iterates over the vector from front to back.
#define FOR_VECTOR_FORWARD(v, i) for (int i = 0; i < v.size(); ++i)

// Iterates over the vector from back to front.
#define FOR_VECTOR_REVERSE(v, i) for (int i = v.size() - 1; i >= 0; --i)

    // Destroys all elements in the vector and emtpies the vector.
    template <typename T>
    static void release(Vector<T*>& v) {
        for (T* v : v) {
            delete v;
        }
        v.release();
    }

    // Erases elements from a vector. Both must be sorted according to compare.
    template <typename T, typename K, typename Compare>
    static void erase(Vector<T>& v, const K* elems, int num, Compare compare) {
        if (num <= 0) return;

        // Work forwards, keeping track of a read and write position.
        T *read = v.begin(), *write = v.begin(), *end = v.end();

        // Skip over elements of the vector until we arrive at the first element
        // to erase.
        while (read != end && compare(*read, *elems)) ++read, ++write;

        // Then, start erasing elements.
        while (read != end && num > 0) {
            if (compare(*read, *elems)) {
                *write = *read;
                ++read, ++write;
            } else if (compare(*elems, *read)) {
                ++elems, --num;
            } else {
                ++read, ++elems, --num;
            }
        }

        // After all elements are erased, there might still be some elements
        // that need to be moved.
        while (read != end) {
            *write = *read;
            ++read, ++write;
        }

        // Apply the new size to the vector.
        v.truncate(write - v.begin());
    }

    // Inserts elements into a vector. Both must be sorted according to compare.
    template <typename T, typename K, typename Compare>
    static void insert(Vector<T>& v, const K* elems, int num, Compare compare) {
        if (num <= 0) return;

        // Make room for the elements that are going to be inserted.
        v.grow(v.size() + num);

        // Work backwards, that way insertion can be done on the fly.
        T *read = v.end() - num - 1, *write = v.end() - 1, *end = v.begin() - 1;
        elems += num - 1;

        // Start inserting elements from read or insert, whichever is the
        // largest.
        while (read != end && num > 0) {
            if (compare(*elems, *read)) {
                *write = *read;
                --read, --write;
            } else {
                *write = *elems;
                --elems, --write, --num;
            }
        }

        // After read end is reached, there might still be some elements that
        // need to be inserted.
        while (num > 0) {
            *write = *elems;
            --elems, --write, --num;
        }
    }

    // Erases matching elements from two vectors for which the predicate is
    // true. Both vectors must be sorted according to compare.
    template <typename T, typename U, typename Compare, typename Predicate>
    static void erasePairs(Vector<T>& a, Vector<U>& b, Compare compare,
                           Predicate pred) {
        for (int i = a.size() - 1, j = b.size() - 1; i >= 0 && j >= 0;) {
            if (compare(a[i], b[j])) {
                --j;
            } else if (compare(b[j], a[i])) {
                --i;
            } else {
                if (pred(a[i], b[j])) {
                    a.erase(i);
                    b.erase(j);
                }
                --i, --j;
            }
        }
    }

};  // Vec.

};  // namespace Vortex

#undef TT