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
	Vector(Vector&& v);
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
	inline T* data() { return myPtr; }

	/// Returns a const pointer to the array of elements.
	inline const T* data() const { return myPtr; }

	/// Returns a pointer to the first element.
	inline T* begin() { return myPtr; }

	/// Returns a const pointer to the first element.
	inline const T* begin() const { return myPtr; }

	/// Returns a pointer to the past-the-end element.
	inline T* end() { return myPtr + myNum; }

	/// Returns a const pointer to the past-the-end element.
	inline const T* end() const { return myPtr + myNum; }

	/// Returns the final element of the vector; does not perform an out-of-bounds check.
	/// To avoid failure, make sure size is non-zero before using this function.
	inline T& back() { return myPtr[myNum - 1]; }

	/// Returns the final element of the vector; does not perform an out-of-bounds check.
	/// To avoid failure, make sure size is non-zero before using this function.
	inline const T& back() const { return myPtr[myNum - 1]; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline T& at(int i) { return myPtr[i]; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline const T& at(int i) const { return myPtr[i]; }

	/// Returns the current number of elements in the vector.
	inline int size() const { return myNum; }

	/// Returns true if the vector has a size of zero.
	inline bool empty() const { return !myNum; }

	/// Return size of allocated storage capacity, expressed in elements.
	inline int capacity() const { return myCap; }

	/// Returns a reference to the value at index i; does not perform an out-of-bounds check.
	inline T& operator [] (int i) { return myPtr[i]; }

	/// Returns a const reference to the value at index i; does not perform an out-of-bounds check.
	inline const T& operator [] (int i) const { return myPtr[i]; }

	/// Appends a value to the end of the vector.
	inline Vector& operator << (const T& v) { push_back(v); return *this; }

private:
	void myGrow(int n);
	T* myPtr;
	int myNum, myCap;
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
	: myPtr(nullptr), myNum(0), myCap(0)
{
}

template <typename T>
Vector<T>::Vector(int n)
	: myPtr(nullptr), myNum(0), myCap(0)
{
	reserve(n);
}

template <typename T>
Vector<T>::Vector(Vector&& v)
	: myPtr(v.myPtr), myNum(v.myNum), myCap(v.myCap)
{
	v.myPtr = nullptr;
	v.myNum = v.myCap = 0;
}

template <typename T>
Vector<T>::Vector(const Vector& v)
	: myPtr(nullptr), myNum(0), myCap(0)
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
	: myPtr(nullptr), myNum(n), myCap(0)
{
	myGrow(myNum);
	for(int i = 0; i < n; ++i) new (myPtr + i) T(v);
}

template <typename T>
Vector<T>::Vector(const T* begin, const T* end)
	: myPtr(nullptr), myNum(end - begin), myCap(0)
{
	myGrow(myNum);
	for(T* p = myPtr; begin != end; ++begin, ++p) new (p)T(*begin);
}

template <typename T>
void Vector<T>::assign(const Vector& o)
{
	if(this != &o)
	{
		clear();
		myNum = o.myNum;
		myGrow(myNum);
		for(int i = 0; i < myNum; ++i) new (myPtr + i) T(o.myPtr[i]);
	}
}

template <typename T>
void Vector<T>::swap(Vector& o)
{
	if(this != &o)
	{
		int n = myNum; myNum = o.myNum; o.myNum = n;
		int c = myCap; myCap = o.myCap; o.myCap = c;
		T*  p = myPtr; myPtr = o.myPtr; o.myPtr = p;
	}
}

template <typename T>
void Vector<T>::clear()
{
	for(int i = 0; i < myNum; ++i) myPtr[i].~T();
	myNum = 0;
}

template <typename T>
void Vector<T>::release()
{
	if(myPtr)
	{
		clear();
		free(myPtr);
		myPtr = nullptr;
		myCap = 0;
	}
}

template <typename T>
void Vector<T>::squeeze()
{
	if(myNum)
	{
		if(myCap > myNum)
		{
			T* src = myPtr;
			myCap = myNum;
			myPtr = (T*)malloc(sizeof(T)*myCap);
			memcpy(myPtr, src, myNum * sizeof(T));
			free(src);
		}
	}
	else release();
}

template <typename T>
void Vector<T>::reserve(int n)
{
	if(n > myCap)
	{
		myPtr = (T*)realloc(myPtr, sizeof(T)*n);
		myCap = n;
	}
}

template <typename T>
void Vector<T>::grow(int n)
{
	if(myNum < n)
	{
		myGrow(n);
		for(int i = myNum; i < n; ++i) new (myPtr + i) T();
		myNum = n;
	}
}

template <typename T>
void Vector<T>::grow(int n, const T& val)
{
	if(myNum < n)
	{
		myGrow(n);
		for(int i = myNum; i < n; ++i) new (myPtr + i) T(val);
		myNum = n;
	}
}


template <typename T>
void Vector<T>::truncate(int n)
{
	if(myNum > n)
	{
		if(n < 0) n = 0;
		for(int i = n; i < myNum; ++i) myPtr[i].~T();
		myNum = n;
	}
}

template <typename T>
void Vector<T>::resize(int n)
{
	if(myNum < n) grow(n); else truncate(n);
}

template <typename T>
void Vector<T>::resize(int n, const T& val)
{
	if(myNum < n)
	{
		myGrow(n);
		for(int i = myNum; i < n; ++i) new (myPtr + i) T(val);
		myNum = n;
	}
	else truncate(n);
}

template <typename T>
T& Vector<T>::append()
{
	if(myNum != myCap)
		new (myPtr + myNum) T(), ++myNum;
	else
		insert(myNum, T(), 1);
	return myPtr[myNum - 1];
}

template <typename T>
void Vector<T>::push_back(const T& v)
{
	if(myNum != myCap)
		new (myPtr + myNum) T(v), ++myNum;
	else
		insert(myNum, v, 1);
}

template <typename T>
void Vector<T>::push_back(T&& v)
{
	if(myNum != myCap)
		new (myPtr + myNum) T(v), ++myNum;
	else
		insert(myNum, v, 1);
}

template <typename T>
void Vector<T>::insert(int i, const T& v, int n)
{
	if(n <= 0) return;
	myGrow(myNum + n);
	if(i >= myNum)
	{
		i = myNum;
	}
	else
	{
		if(i < 0) i = 0;
		memmove(myPtr + i + n, myPtr + i, sizeof(T)*(myNum - i));
	}
	for(int j = 0; j < n; ++j)
	{
		new (myPtr + i + j) T(v);
	}
	myNum += n;
}

template <typename T>
void Vector<T>::insert(int i, const T* v, int n)
{
	if(n <= 0) return;
	myGrow(myNum + n);
	if(i >= myNum)
	{
		i = myNum;
	}
	else
	{
		if(i < 0) i = 0;
		memmove(myPtr + i + n, myPtr + i, sizeof(T)*(myNum - i));
	}
	for(int j = 0; j < n; ++j)
	{
		new (myPtr + i + j) T(v[j]);
	}
	myNum += n;
}

template <typename T>
void Vector<T>::erase(int i)
{
	if(i >= 0 && i < myNum)
	{
		int end = i + 1;
		myPtr[i].~T();
		memmove(myPtr + i, myPtr + end, sizeof(T)*(myNum - end));
		--myNum;
	}
}

template <typename T>
void Vector<T>::erase(int begin, int end)
{
	if(begin < 0) begin = 0;
	if(end > myNum) end = myNum;
	if(begin < end)
	{
		for(int i = begin; i < end; ++i) myPtr[i].~T();
		memmove(myPtr + begin, myPtr + end, sizeof(T)*(myNum - end));
		myNum -= end - begin;
	}
}

template <typename T>
void Vector<T>::erase_values(const T& v)
{
	for(int i = myNum - 1; i >= 0; --i)
	{
		if(myPtr[i] == v) erase(i);
	}
}

template <typename T>
void Vector<T>::pop_back()
{
	if(myNum) myPtr[--myNum].~T();
}

template <typename T>
bool Vector<T>::contains(const T& v) const
{
	return find(v) != myNum;
}

template <typename T>
int Vector<T>::find(const T& v, int i) const
{
	while(i < myNum && myPtr[i] != v) ++i;
	return i;
}

template <typename T>
void Vector<T>::myGrow(int n)
{
	if(myCap < n)
	{
		myCap <<= 1;
		if(myCap < n) myCap = n;
		myPtr = (T*)realloc(myPtr, sizeof(T)*myCap);
	}
}

}; // namespace Vortex
