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

	void write8(const void* val);
	void write16(const void* val);
	void write32(const void* val);
	void write64(const void* val);

	void writeNum(uint num);
	void writeStr(StringRef str);

	template <typename T>
	inline void write(const T& val)
	{
		write(&val, sizeof(T));
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

	void read8(void* out);
	void read16(void* out);
	void read32(void* out);
	void read64(void* out);

	uint readNum();
	String readStr();

	void readNum(uint& num);
	void readStr(String& str);

	template <typename T>
	inline void read(T& out)
	{
		read(&out, sizeof(T));
	}

	template <typename T>
	inline T read()
	{
		T out = T();
		read(&out, sizeof out);
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
