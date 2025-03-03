#pragma once

#include <Precomp.h>

namespace AV {

// Provides serialization of arbitrary data to a binary buffer.
class Serializer
{
public:
	~Serializer();

	// Outputs to an internal buffer, which grows as more data is written.
	Serializer();

	// Outputs to an external buffer, and stops writing when the buffer is full.
	Serializer(void* buffer, size_t numBytes);

	// Jumps ahead in the buffer without writing data.
	void skip(size_t numBytes);

	// Writes some amount of bytes of raw data.
	void write(const void* data, size_t numBytes);

	// Writes 1 byte of raw data.
	void write8(const void* data);

	// Writes 2 bytes of raw data.
	void write16(const void* data);

	// Writes 4 bytes of raw data.
	void write32(const void* data);
	
	// Writes 8 bytes of raw data.
	void write64(const void* data);

	// Writes a number, using less bytes for smaller values.
	void writeNum(uint64_t value);

	// Writes a string as a length/data pair.
	void writeStr(stringref str);

	// Writes a value with an arbitrary type as raw data.
	template <typename T>
	inline void write(const T& v) { myWrite<sizeof(T)>(&v); }

	// Returns true if all write operations so far have succeeded, false otherwise.
	bool success() const { return mySuccess; }

	// Returns the total amount of bytes written.
	size_t size() const { return mySize; }

	// Returns a pointer to the start of the written data.
	const uchar* data() const { return myData; }

private:
	template <size_t Size>
	inline void myWrite(const void* data) { write(data, Size); }

	template <>
	inline void myWrite<1>(const void* data) { write8(data); }

	template <>
	inline void myWrite<2>(const void* data) { write16(data); }

	template <>
	inline void myWrite<4>(const void* data) { write32(data); }

	template <>
	inline void myWrite<8>(const void* data) { write64(data); }

	uchar* myData;
	size_t mySize;
	size_t myCap;
	bool myIsOwner;
	bool mySuccess;
};

// Provides deserialization of arbitrary data from a binary buffer.
class Deserializer
{
public:
	~Deserializer();

	// Reads from a buffer of unknown size, only use this if you know exactly what data you're reading.
	Deserializer(const void* buffer);

	// Reads from a buffer of <numBytes> in size, and stops reading when the buffer end is reached.
	Deserializer(const void* buffer, size_t numBytes);

	// Skips over a number of bytes.
	void skip(size_t numBytes);

	// Reads a number of bytes of raw data.
	void read(void* output, size_t numBytes);

	// Reads 1 byte of raw data.
	void read8(void* output);

	// Reads 2 bytes of raw data.
	void read16(void* output);

	// Reads 4 bytes of raw data.
	void read32(void* output);

	// Reads 8 bytes of raw data.
	void read64(void* output);

	// Reads an encoded number, as written by Serializer::writeNum.
	uint readNum();

	// Reads an encoded number, as written by Serializer::writeNum.
	void readNum(uint& value);

	// Reads an encoded string, as written by Serializer::writeStr.
	std::string readStr();

	// Reads an encoded string, as written by Serializer::writeStr.
	void readStr(std::string& str);

	// Reads a value with an arbitrary type as raw data.
	template <typename T>
	inline void read(T& out) { myRead<sizeof(T)>(&out); }

	// Reads a value with an arbitrary type as raw data.
	template <typename T>
	inline T read() { T r=T(); myRead<sizeof(T)>(&r); return r; }

	// Skips a value with an arbitrary type as raw data.
	template <typename T>
	inline void skip() { skip(sizeof(T)); }

	// Sets success to false and skips to the stream end.
	void invalidate() { mySuccess = false; myPos = myEnd; }

	// Returns true if all previous read operations have succeeded.
	bool success() const { return mySuccess; }

	// Returns true if one or more previous read operations have failed.
	bool failed() const { return !mySuccess; }

	// Returns the number of bytes left to read.
	size_t bytesLeft() const { return myEnd - myPos; }

	// Returns the current read position.
	const uchar* pos() const { return myPos; }

private:
	template <size_t Size>
	inline void myRead(void* out) { read(out, Size); }

	template <>
	inline void myRead<1>(void* out) { read8(out); }

	template <>
	inline void myRead<2>(void* out) { read16(out); }

	template <>
	inline void myRead<4>(void* out) { read32(out); }

	template <>
	inline void myRead<8>(void* out) { read64(out); }

	const uchar* myPos;
	const uchar* myEnd;
	bool mySuccess;
};

} // namespace AV