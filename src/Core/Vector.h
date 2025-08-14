#pragma once

#include <Core/Core.h>

#include <stdlib.h>
#include <string.h>
#include <new>

namespace Vortex {

template <typename T>
class Vector
{
public:
	~Vector();
	Vector();
	Vector(Vector&& v) noexcept;
	Vector(const Vector& v);
	Vector& operator = (Vector v);

	/// Constructs an empty vector. Reserves memory for the given number of elements.
	Vector(int capacity);

	/// Constructs a vector of the given size, with all elements copy constructed from v.
	Vector(int size, const T& value);

	/// Constructs a vector from a range of values.
	Vector(const T* begin, const T* end);

	/// Sets the vector contents by copying the elements of another vector.
	void assign(const Vector& other);

	/// Swaps the contents with another vector.
	void swap(Vector& other);

	/// Removes all the elements from the vector.
	void clear();

	/// Removes all elements from the vector and releases the reserved memory.
	void release();

	/// Reduces the capacity to the minimum size required to store all current elements.
	void squeeze();

	/// Reserves memory for the given number of elements.
	void reserve(int capacity);

	/// Makes sure the vector contains at least minCount elements.
	void grow(int minCount);

	/// Makes sure the vector contains at least minCount elements.
	void grow(int minCount, const T& val);

	/// Makes sure the vector contains at most maxCount elements.
	void truncate(int maxCount);

	/// Resizes the vector to count elements.
	void resize(int count);

	/// Resizes the vector to count elements.
	void resize(int count, const T& val);

	/// Appends a default constructed elements and returns a reference to it.
	T& append();

	/// Appends a value to the end of the vector.
	void push_back(const T& val);

	/// Appends a value to the end of the vector.
	void push_back(T&& val);

	/// Inserts a value at index pos in the vector.
	void insert(int pos, const T& val, int num);

	/// Inserts an array of count values at index pos in the vector.
	void insert(int pos, const T* val, int count);

	/// Removes the element at index pos from the vector.
	void erase(int pos);

	/// Removes all elements with indices in the range [begin, end).
	void erase(int begin, int end);

	/// Removes all elements that are equal to value from the vector.
	void erase_values(const T& value);

	/// Removes the last element from the vector.
	void pop_back();

	/// Returns true if the vector contains one or more elements that equal value.
	bool contains(const T& value) const;

	/// Returns the index of the first element on or after pos that equals value.
	/// If no matching element is found, the vector size is returned.
	int find(const T& value, int pos = 0) const;

	/// Returns a pointer to the array of elements.
	inline T* data() { return data_; }

	/// Returns a const pointer to the array of elements.
	inline const T* data() const { return data_; }

	/// Returns a pointer to the first element.
	inline T* begin() { return data_; }

	/// Returns a const pointer to the first element.
	inline const T* begin() const { return data_; }

	/// Returns a pointer to the past-the-end element.
	inline T* end() { return data_ + size_; }

	/// Returns a const pointer to the past-the-end element.
	inline const T* end() const { return data_ + size_; }

	/// Returns the final element of the vector; does not perform an out-of-bounds check.
	/// To avoid failure, make sure size is non-zero before using this function.
	inline T& back() { return data_[size_ - 1]; }

	/// Returns the final element of the vector; does not perform an out-of-bounds check.
	/// To avoid failure, make sure size is non-zero before using this function.
	inline const T& back() const { return data_[size_ - 1]; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline T& at(int i) { return data_[i]; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline const T& at(int i) const { return data_[i]; }

	/// Returns the current number of elements in the vector.
	inline int size() const { return size_; }

	/// Returns true if the vector has a size of zero.
	inline bool empty() const { return !size_; }

	/// Return size of allocated storage capacity, expressed in elements.
	inline int capacity() const { return capacity_; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline T& operator [] (int i) { return data_[i]; }

	/// Returns a const reference to the value at index i; does not perform an out-of-bounds check.
	inline const T& operator [] (int i) const { return data_[i]; }

	/// Appends a value to the end of the vector.
	inline Vector& operator << (const T& v) { push_back(v); return *this; }

private:
	void EnsureCapacity(int n);
	T* data_;
	int size_, capacity_;
};

// ================================================================================================
// Anything below this line is used internally and is not part of the API.

template <typename T>
Vector<T>::~Vector()
{
	release();
}

template <typename T>
Vector<T>::Vector()
	: data_(nullptr), size_(0), capacity_(0)
{
}

template <typename T>
Vector<T>::Vector(int n)
	: data_(nullptr), size_(0), capacity_(0)
{
	reserve(n);
}

template <typename T>
Vector<T>::Vector(Vector&& v) noexcept
	: data_(v.data_), size_(v.size_), capacity_(v.capacity_)
{
	v.data_ = nullptr;
	v.size_ = v.capacity_ = 0;
}

template <typename T>
Vector<T>::Vector(const Vector& v)
	: data_(nullptr), size_(0), capacity_(0)
{
	assign(v);
}

template <typename T>
Vector<T>& Vector<T>::operator = (Vector<T> v)
{
	swap(v);
	return *this;
}

template <typename T>
Vector<T>::Vector(int n, const T& v)
	: data_(nullptr), size_(n), capacity_(0)
{
	EnsureCapacity(size_);
	for(int i = 0; i < n; ++i) new (data_ + i) T(v);
}

template <typename T>
Vector<T>::Vector(const T* begin, const T* end)
	: data_(nullptr), size_(end - begin), capacity_(0)
{
	EnsureCapacity(size_);
	for(T* p = data_; begin != end; ++begin, ++p) new (p)T(*begin);
}

template <typename T>
void Vector<T>::assign(const Vector& o)
{
	if(this != &o)
	{
		clear();
		size_ = o.size_;
		EnsureCapacity(size_);
		for(int i = 0; i < size_; ++i) new (data_ + i) T(o.data_[i]);
	}
}

template <typename T>
void Vector<T>::swap(Vector& o)
{
	if(this != &o)
	{
		int n = size_; size_ = o.size_; o.size_ = n;
		int c = capacity_; capacity_ = o.capacity_; o.capacity_ = c;
		T*  p = data_; data_ = o.data_; o.data_ = p;
	}
}

template <typename T>
void Vector<T>::clear()
{
	for(int i = 0; i < size_; ++i) data_[i].~T();
	size_ = 0;
}

template <typename T>
void Vector<T>::release()
{
	if(data_)
	{
		clear();
		free(data_);
		data_ = nullptr;
		capacity_ = 0;
	}
}

template <typename T>
void Vector<T>::squeeze()
{
	if(size_)
	{
		if(capacity_ > size_)
		{
			T* src = data_;
			capacity_ = size_;
			data_ = (T*)malloc(sizeof(T)*capacity_);
			memcpy(data_, src, size_ * sizeof(T));
			free(src);
		}
	}
	else release();
}

template <typename T>
void Vector<T>::reserve(int n)
{
	if(n > capacity_)
	{
		data_ = (T*)realloc(data_, sizeof(T)*n);
		capacity_ = n;
	}
}

template <typename T>
void Vector<T>::grow(int n)
{
	if(size_ < n)
	{
		EnsureCapacity(n);
		for(int i = size_; i < n; ++i) new (data_ + i) T();
		size_ = n;
	}
}

template <typename T>
void Vector<T>::grow(int n, const T& val)
{
	if(size_ < n)
	{
		EnsureCapacity(n);
		for(int i = size_; i < n; ++i) new (data_ + i) T(val);
		size_ = n;
	}
}


template <typename T>
void Vector<T>::truncate(int n)
{
	if(size_ > n)
	{
		if(n < 0) n = 0;
		for(int i = n; i < size_; ++i) data_[i].~T();
		size_ = n;
	}
}

template <typename T>
void Vector<T>::resize(int n)
{
	if(size_ < n) grow(n); else truncate(n);
}

template <typename T>
void Vector<T>::resize(int n, const T& val)
{
	if(size_ < n)
	{
		EnsureCapacity(n);
		for(int i = size_; i < n; ++i) new (data_ + i) T(val);
		size_ = n;
	}
	else truncate(n);
}

template <typename T>
T& Vector<T>::append()
{
	if(size_ != capacity_)
		new (data_ + size_) T(), ++size_;
	else
		insert(size_, T(), 1);
	return data_[size_ - 1];
}

template <typename T>
void Vector<T>::push_back(const T& v)
{
	if(size_ != capacity_)
		new (data_ + size_) T(v), ++size_;
	else
		insert(size_, v, 1);
}

template <typename T>
void Vector<T>::push_back(T&& v)
{
	if(size_ != capacity_)
		new (data_ + size_) T(v), ++size_;
	else
		insert(size_, v, 1);
}

template <typename T>
void Vector<T>::insert(int i, const T& v, int n)
{
	if(n <= 0) return;
	EnsureCapacity(size_ + n);
	if(i >= size_)
	{
		i = size_;
	}
	else
	{
		if(i < 0) i = 0;
		memmove(data_ + i + n, data_ + i, sizeof(T)*(size_ - i));
	}
	for(int j = 0; j < n; ++j)
	{
		new (data_ + i + j) T(v);
	}
	size_ += n;
}

template <typename T>
void Vector<T>::insert(int i, const T* v, int n)
{
	if(n <= 0) return;
	EnsureCapacity(size_ + n);
	if(i >= size_)
	{
		i = size_;
	}
	else
	{
		if(i < 0) i = 0;
		memmove(data_ + i + n, data_ + i, sizeof(T)*(size_ - i));
	}
	for(int j = 0; j < n; ++j)
	{
		new (data_ + i + j) T(v[j]);
	}
	size_ += n;
}

template <typename T>
void Vector<T>::erase(int i)
{
	if(i >= 0 && i < size_)
	{
		int end = i + 1;
		data_[i].~T();
		memmove(data_ + i, data_ + end, sizeof(T)*(size_ - end));
		--size_;
	}
}

template <typename T>
void Vector<T>::erase(int begin, int end)
{
	if(begin < 0) begin = 0;
	if(end > size_) end = size_;
	if(begin < end)
	{
		for(int i = begin; i < end; ++i) data_[i].~T();
		memmove(data_ + begin, data_ + end, sizeof(T)*(size_ - end));
		size_ -= end - begin;
	}
}

template <typename T>
void Vector<T>::erase_values(const T& v)
{
	for(int i = size_ - 1; i >= 0; --i)
	{
		if(data_[i] == v) erase(i);
	}
}

template <typename T>
void Vector<T>::pop_back()
{
	if(size_) data_[--size_].~T();
}

template <typename T>
bool Vector<T>::contains(const T& v) const
{
	return find(v) != size_;
}

template <typename T>
int Vector<T>::find(const T& v, int i) const
{
	while(i < size_ && data_[i] != v) ++i;
	return i;
}

template <typename T>
void Vector<T>::EnsureCapacity(int n)
{
	if(capacity_ < n)
	{
		capacity_ <<= 1;
		if(capacity_ < n) capacity_ = n;
		auto new_data_ = (T*)realloc(data_, sizeof(T) * capacity_);
		if (new_data_)
		{
			data_ = new_data_;
		}
		else
		{
			throw std::bad_alloc();
		}
	}
}

}; // namespace Vortex
