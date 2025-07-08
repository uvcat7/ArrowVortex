#include <Core/ByteStream.h>
#include <Core/String.h>
#include <Core/Utils.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

namespace Vortex {

// ================================================================================================
// WriteStream.

WriteStream::WriteStream()
{
	buffer_ = (uchar*)malloc(128);
	current_size_ = 0;
	capacity_ = 128;
	is_external_buffer_ = false;
	is_write_successful_ = true;
}

WriteStream::WriteStream(void* out, int bytes)
{
	buffer_ = (uchar*)out;
	current_size_ = 0;
	capacity_ = bytes;
	is_external_buffer_ = true;
	is_write_successful_ = true;
}

WriteStream::~WriteStream()
{
	if(!is_external_buffer_) free(buffer_);
}

void WriteStream::write(const void* in, int bytes)
{
	int newSize = current_size_ + bytes;
	if(newSize <= capacity_)
	{
		memcpy(buffer_ + current_size_, in, bytes);
		current_size_ = newSize;
	}
	else if(!is_external_buffer_)
	{
		capacity_ = max(capacity_ << 1, newSize);
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		memcpy(buffer_ + current_size_, in, bytes);
		current_size_ = newSize;
	}
	else
	{
		is_write_successful_ = false;
	}
}

void WriteStream::write8(const void* val)
{
	if(current_size_ < capacity_)
	{
		buffer_[current_size_] = *(const uint8_t*)val;
		++current_size_;
	}
	else if(!is_external_buffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		buffer_[current_size_] = *(const uint8_t*)val;
		++current_size_;
	}
	else
	{
		is_write_successful_ = false;
	}
}

void WriteStream::write16(const void* val)
{
	int newSize = current_size_ + sizeof(uint16_t);
	if(newSize <= capacity_)
	{
		*(uint16_t*)(buffer_ + current_size_) = *(const uint16_t*)val;
		current_size_ = newSize;
	}
	else if(!is_external_buffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint16_t*)(buffer_ + current_size_) = *(const uint16_t*)val;
		current_size_ = newSize;
	}
	else
	{
		is_write_successful_ = false;
	}
}

void WriteStream::write32(const void* val)
{
	int newSize = current_size_ + sizeof(uint32_t);
	if(newSize <= capacity_)
	{
		*(uint32_t*)(buffer_ + current_size_) = *(const uint32_t*)val;
		current_size_ = newSize;
	}
	else if(!is_external_buffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint32_t*)(buffer_ + current_size_) = *(const uint32_t*)val;
		current_size_ = newSize;
	}
	else
	{
		is_write_successful_ = false;
	}
}

void WriteStream::write64(const void* val)
{
	int newSize = current_size_ + sizeof(uint64_t);
	if(newSize <= capacity_)
	{
		*(uint64_t*)(buffer_ + current_size_) = *(const uint64_t*)val;
		current_size_ = newSize;
	}
	else if(!is_external_buffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint64_t*)(buffer_ + current_size_) = *(const uint64_t*)val;
		current_size_ = newSize;
	}
	else
	{
		is_write_successful_ = false;
	}
}

void WriteStream::writeNum(uint num)
{
	if(num < 0x80)
	{
		write8(&num);
	}
	else
	{
		uchar buf[8];
		uint index = 0;
		while(num >= 0x80)
		{
			buf[index] = (num & 0x7F) | 0x80;
			num >>= 7;
			++index;
		}
		buf[index] = num;
		write(buf, index + 1);
	}
}

void WriteStream::writeStr(StringRef str)
{
	writeNum(str.len());
	write(str.str(), str.len());
}

// ================================================================================================
// ReadStream.

ReadStream::ReadStream(const void* in, int bytes)
{
	read_position_ = (const uchar*)in;
	end_position_ = read_position_ + bytes;
	is_read_successful_ = true;
}

ReadStream::~ReadStream()
{
}

void ReadStream::skip(int bytes)
{
	if(read_position_ + bytes <= end_position_)
	{
		read_position_ += bytes;
	}
	else
	{
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}

void ReadStream::read(void* out, int bytes)
{
	if(read_position_ + bytes <= end_position_)
	{
		memcpy(out, read_position_, bytes);
		read_position_ += bytes;
	}
	else
	{
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}

void ReadStream::read8(void* out)
{
	if(read_position_ != end_position_)
	{
		*(uint8_t*)out = *read_position_;
		++read_position_;
	}
	else
	{
		*(uint8_t*)out = 0;
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}

void ReadStream::read16(void* out)
{
	auto newPos = read_position_ + sizeof(uint16_t);
	if(newPos <= end_position_)
	{
		*(uint16_t*)out = *(const uint16_t*)read_position_;
		read_position_ = newPos;
	}
	else
	{
		*(uint16_t*)out = 0;
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}


void ReadStream::read32(void* out)
{
	auto newPos = read_position_ + sizeof(uint32_t);
	if(newPos <= end_position_)
	{
		*(uint32_t*)out = *(const uint32_t*)read_position_;
		read_position_ = newPos;
	}
	else
	{
		*(uint32_t*)out = 0;
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}

void ReadStream::read64(void* out)
{
	auto newPos = read_position_ + sizeof(uint64_t);
	if(newPos <= end_position_)
	{
		*(uint64_t*)out = *(const uint64_t*)read_position_;
		read_position_ = newPos;
	}
	else
	{
		*(uint64_t*)out = 0;
		read_position_ = end_position_;
		is_read_successful_ = false;
	}
}

uint ReadStream::readNum()
{
	uint32_t out;
	if(read_position_ != end_position_)
	{
		if(*read_position_ < 0x80)
		{
			out = *read_position_;
			++read_position_;
		}
		else
		{
			out = *read_position_ & 0x7F;
			uint shift = 7;
			while(true)
			{
				if(++read_position_ == end_position_)
				{
					out = 0;
					is_read_successful_ = false;
					break;
				}
				uint byte = *read_position_;
				if((byte & 0x80) == 0)
				{
					out |= byte << shift;
					++read_position_;
					break;
				}
				out |= (byte & 0x7F) << shift;
				shift += 7;
			}
		}
	}
	else
	{
		out = 0;
		is_read_successful_ = false;
	}
	return out;
}

String ReadStream::readStr()
{
	String out;
	uint len = readNum();
	auto newPos = read_position_ + len;
	if(newPos <= end_position_)
	{
		auto str = read_position_;
		read_position_ = newPos;
		return String((const char*)str, len);
	}
	read_position_ = end_position_;
	is_read_successful_ = false;
	return String();
}

void ReadStream::readNum(uint& num)
{
	num = readNum();
}

void ReadStream::readStr(String& str)
{
	str = readStr();
}

}; // namespace Vortex
