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

	template <unsigned int S>
	inline void writeSz(const void* val)
	{
		write(val, S);
	}

	template <>
	inline void writeSz<1>(const void* val)
	{
		write8(val);
	}

	template <>
	inline void writeSz<2>(const void* val)
	{
		write16(val);
	}

	template <>
	inline void writeSz<4>(const void* val)
	{
		write32(val);
	}

	template <>
	inline void writeSz<8>(const void* val)
	{
		write64(val);
	}

	template <typename T>
	inline void write(const T& val)
	{
		writeSz<sizeof(T)>(&val);
	}

	// Returns true if all write operations have succeeded.
	bool success() const { return is_write_successful_; }

	// Returns the total amount of written bytes.
	int size() const { return current_size_; }

	// Returns a pointer to the start of the written data.
	const uchar* data() const { return buffer_; }

private:
	uchar* buffer_;
	int current_size_, capacity_;
	bool is_external_buffer_, is_write_successful_;
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

	template <size_t S>
	inline void readSz(void* out)
	{
		read(out, S);
	}

	template <>
	inline void readSz<1>(void* out)
	{
		read8(out);
	}

	template <>
	inline void readSz<2>(void* out)
	{
		read16(out);
	}

	template <>
	inline void readSz<4>(void* out)
	{
		read32(out);
	}

	template <>
	inline void readSz<8>(void* out)
	{
		read64(out);
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
	void invalidate() { is_read_successful_ = false; read_position_ = end_position_; }

	// Returns true if all read operations have succeeded.
	bool success() const { return is_read_successful_; }

	// Returns the number of bytes left to read.
	size_t bytesleft() const { return end_position_ - read_position_; }

	// Returns the current read position.
	const uchar* pos() const { return read_position_; }

private:
	const uchar* read_position_, *end_position_;
	bool is_read_successful_;
};

}; // namespace Vortex
