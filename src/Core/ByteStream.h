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

	void writeNum(uint32_t num);
	void writeStr(const std::string& str);

	template <typename T>
	inline void write(const T& val)
	{
		switch (sizeof(T)) {
			case 1:
				write8(&val);
				break;

			case 2:
				write16(&val);
				break;

			case 4:
				write32(&val);
				break;

			case 8:
				write64(&val);
				break;

			default:
				write(&val, static_cast<int>(sizeof(T)));
		}
	}

	// Returns true if all write operations have succeeded.
	bool success() const { return is_write_successful_; }

	// Returns the total amount of written bytes.
	int size() const { return current_size_; }

	// Returns a pointer to the start of the written data.
	const uint8_t* data() const { return buffer_; }

private:
	uint8_t* buffer_;
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

	uint32_t readNum();
	std::string readStr();

	void readNum(uint32_t& num);
	void readStr(std::string& str);

	template <typename T>
	inline void read(T& out)
	{
		switch (sizeof(T)) {
			case 1:
				read8(&out);
				break;
			
			case 2:
				read16(&out);
				break;

			case 4:
				read32(&out);
				break;

			case 8:
				read64(&out);
				break;

			default:
				read(&out, static_cast<int>(sizeof(T)));
		}
	}

	template <typename T>
	inline T read()
	{
		T out = T();
		read(out);
		return out;
	}

	// Sets success to false and skips to the stream end.
	void invalidate() { is_read_successful_ = false; read_position_ = end_position_; }

	// Returns true if all read operations have succeeded.
	bool success() const { return is_read_successful_; }

	// Returns the number of bytes left to read.
	size_t bytesleft() const { return end_position_ - read_position_; }

	// Returns the current read position.
	const uint8_t* pos() const { return read_position_; }

private:
	const uint8_t* read_position_, *end_position_;
	bool is_read_successful_;
};

}; // namespace Vortex
