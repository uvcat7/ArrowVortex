#pragma once

#include <Core/Core.h>
#include <Core/Vector.h>

namespace Vortex {

struct WriteStream
{
public:
	WriteStream();
	WriteStream(void* out, int bytes);
	~WriteStream();

	void write(const void* in, int bytes);

	void writeNum(uint num);
	void writeStr(StringRef str);

	template <unsigned int S>
	inline void writeSz(const void* val)
	{
		write(val, S);
	}

	template <typename T>
	inline void write(const T& val)
	{
		writeSz<sizeof(T)>(&val);
	}

	// Returns true if all write operations have succeeded.
	bool success() const { return mySuccess; }

	// Returns the total amount of written bytes.
	int size() const { return mySize; }

	// Returns a pointer to the start of the written data.
	const uchar* data() const { return myBuf; }

private:
	uchar* myBuf;
	int mySize, myCap;
	bool myExtern, mySuccess;
};

struct ReadStream
{
public:
	ReadStream(const void* in, int bytes);
	~ReadStream();

	void skip(int bytes);

	void read(void* out, int bytes);

	uint readNum();
	String readStr();

	void readNum(uint& num);
	void readStr(String& str);

	template <size_t S>
	inline void readSz(void* out)
	{
		read(out, S);
	}

	template <typename T>
	inline void read(T& out)
	{
		readSz<sizeof(T)>(&out);
	}

	template <typename T>
	inline T read()
	{
		T out = T();
		readSz<sizeof(T)>(&out);
		return out;
	}

	// Sets success to false and skips to the stream end.
	void invalidate() { mySuccess = false; myPos = myEnd; }

	// Returns true if all read operations have succeeded.
	bool success() const { return mySuccess; }

	// Returns the number of bytes left to read.
	size_t bytesleft() const { return myEnd - myPos; }

	// Returns the current read position.
	const uchar* pos() const { return myPos; }

private:
	const uchar* myPos, *myEnd;
	bool mySuccess;
};

}; // namespace Vortex
