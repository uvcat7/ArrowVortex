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

	template <typename T>
	inline void write(const T& val)
	{
		write(&val, sizeof(T));
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
		read(&out, sizeof(T));
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
