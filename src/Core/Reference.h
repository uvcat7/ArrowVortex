#pragma once

#include <Core/Core.h>

#include <stdlib.h>
#include <new>

namespace Vortex {

// ================================================================================================

template <typename T>
class Reference
{
public:
	~Reference();
	Reference();
	Reference(const Reference& v);
	Reference& operator = (const Reference& v);

	/// Dereferences the current reference and deletes it if count reaches zero.
	void destroy();

	/// Destroys the current reference, and creates a new reference.
	T* create();

	/// Destroys the current reference, and creates a new reference to a copy of value.
	void create(const T& value);

	/// Destroys the current reference, and references the value of another reference.
	void copy(const Reference<T>& other);

	/// Returns true if the current value is null.
	inline bool null() const { return !myPtr; }
	
	/// Returns the number of references to the current value.
	inline int count() const { return myPtr ? *((int*)myPtr - 1) : 0; }

	/// Returns a pointer to the current value.
	inline operator T*() { return myPtr; }

	/// Returns a const pointer to the current value.
	inline operator T*() const { return myPtr; }

	/// Returns a pointer to the current value.
	inline T* operator ->() { return myPtr; }

	/// Returns a const pointer to the current value.
	inline const T* operator ->() const { return myPtr; }

	/// Returns a pointer to the current value.
	inline T* get() { return myPtr; }

	/// Returns a const pointer to the current value.
	inline const T* get() const { return myPtr; }

private:
	void myCreateRef();
	T* myPtr; // TODO rename this to something more descriptive, but need to make sure renaming it won't break stuff
};

// ================================================================================================
// Anything below this line is used internally and is not part of the API.

template <typename T>
Reference<T>::~Reference()
{
	destroy();
}

template <typename T>
Reference<T>::Reference()
	: myPtr(nullptr)
{
}

template <typename T>
Reference<T>::Reference(const Reference& r)
	: myPtr(nullptr)
{
	copy(r);
}

template <typename T>
Reference<T>& Reference<T>::operator = (const Reference& r)
{
	copy(r);
	return *this;
}

template <typename T>
T* Reference<T>::create()
{
	myCreateRef();
	new (myPtr) T();
	return myPtr;
}

template <typename T>
void Reference<T>::create(const T& v)
{
	myCreateRef();
	new (myPtr)T(v);
}

template <typename T>
void Reference<T>::destroy()
{
	if(myPtr)
	{
		int* ref = (int*)myPtr - 1;
		if(--*ref == 0)
		{
			myPtr->~T();
			delete ref;
		}
		myPtr = nullptr;
	}
}

template <typename T>
void Reference<T>::copy(const Reference& o)
{
	if(myPtr != o.myPtr)
	{
		destroy();
		myPtr = o.myPtr;
		++*((int*)myPtr - 1);
	}
}

template <typename T>
void Reference<T>::myCreateRef()
{
	destroy();
	int* ref = (int*)malloc(sizeof(int) + sizeof(T));
	*ref = 1;
	myPtr = (T*)(ref + 1);
}

}; // namespace Vortex
