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
	currentSize_ = 0;
	capacity_ = 128;
	isExternalBuffer_ = false;
	isWriteSuccessful_ = true;
}

WriteStream::WriteStream(void* out, int bytes)
{
	buffer_ = (uchar*)out;
	currentSize_ = 0;
	capacity_ = bytes;
	isExternalBuffer_ = true;
	isWriteSuccessful_ = true;
}

WriteStream::~WriteStream()
{
	if(!isExternalBuffer_) free(buffer_);
}

void WriteStream::write(const void* in, int bytes)
{
	int newSize = currentSize_ + bytes;
	if(newSize <= capacity_)
	{
		memcpy(buffer_ + currentSize_, in, bytes);
		currentSize_ = newSize;
	}
	else if(!isExternalBuffer_)
	{
		capacity_ = max(capacity_ << 1, newSize);
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		memcpy(buffer_ + currentSize_, in, bytes);
		currentSize_ = newSize;
	}
	else
	{
		isWriteSuccessful_ = false;
	}
}

void WriteStream::write8(const void* val)
{
	if(currentSize_ < capacity_)
	{
		buffer_[currentSize_] = *(const uint8_t*)val;
		++currentSize_;
	}
	else if(!isExternalBuffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		buffer_[currentSize_] = *(const uint8_t*)val;
		++currentSize_;
	}
	else
	{
		isWriteSuccessful_ = false;
	}
}

void WriteStream::write16(const void* val)
{
	int newSize = currentSize_ + sizeof(uint16_t);
	if(newSize <= capacity_)
	{
		*(uint16_t*)(buffer_ + currentSize_) = *(const uint16_t*)val;
		currentSize_ = newSize;
	}
	else if(!isExternalBuffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint16_t*)(buffer_ + currentSize_) = *(const uint16_t*)val;
		currentSize_ = newSize;
	}
	else
	{
		isWriteSuccessful_ = false;
	}
}

void WriteStream::write32(const void* val)
{
	int newSize = currentSize_ + sizeof(uint32_t);
	if(newSize <= capacity_)
	{
		*(uint32_t*)(buffer_ + currentSize_) = *(const uint32_t*)val;
		currentSize_ = newSize;
	}
	else if(!isExternalBuffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint32_t*)(buffer_ + currentSize_) = *(const uint32_t*)val;
		currentSize_ = newSize;
	}
	else
	{
		isWriteSuccessful_ = false;
	}
}

void WriteStream::write64(const void* val)
{
	int newSize = currentSize_ + sizeof(uint64_t);
	if(newSize <= capacity_)
	{
		*(uint64_t*)(buffer_ + currentSize_) = *(const uint64_t*)val;
		currentSize_ = newSize;
	}
	else if(!isExternalBuffer_)
	{
		capacity_ <<= 1;
		buffer_ = (uchar*)realloc(buffer_, capacity_);
		*(uint64_t*)(buffer_ + currentSize_) = *(const uint64_t*)val;
		currentSize_ = newSize;
	}
	else
	{
		isWriteSuccessful_ = false;
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
	readPosition_ = (const uchar*)in;
	endPosition_ = readPosition_ + bytes;
	isReadSuccessful_ = true;
}

ReadStream::~ReadStream()
{
}

void ReadStream::skip(int bytes)
{
	if(readPosition_ + bytes <= endPosition_)
	{
		readPosition_ += bytes;
	}
	else
	{
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}

void ReadStream::read(void* out, int bytes)
{
	if(readPosition_ + bytes <= endPosition_)
	{
		memcpy(out, readPosition_, bytes);
		readPosition_ += bytes;
	}
	else
	{
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}

void ReadStream::read8(void* out)
{
	if(readPosition_ != endPosition_)
	{
		*(uint8_t*)out = *readPosition_;
		++readPosition_;
	}
	else
	{
		*(uint8_t*)out = 0;
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}

void ReadStream::read16(void* out)
{
	auto newPos = readPosition_ + sizeof(uint16_t);
	if(newPos <= endPosition_)
	{
		*(uint16_t*)out = *(const uint16_t*)readPosition_;
		readPosition_ = newPos;
	}
	else
	{
		*(uint16_t*)out = 0;
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}


void ReadStream::read32(void* out)
{
	auto newPos = readPosition_ + sizeof(uint32_t);
	if(newPos <= endPosition_)
	{
		*(uint32_t*)out = *(const uint32_t*)readPosition_;
		readPosition_ = newPos;
	}
	else
	{
		*(uint32_t*)out = 0;
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}

void ReadStream::read64(void* out)
{
	auto newPos = readPosition_ + sizeof(uint64_t);
	if(newPos <= endPosition_)
	{
		*(uint64_t*)out = *(const uint64_t*)readPosition_;
		readPosition_ = newPos;
	}
	else
	{
		*(uint64_t*)out = 0;
		readPosition_ = endPosition_;
		isReadSuccessful_ = false;
	}
}

uint ReadStream::readNum()
{
	uint32_t out;
	if(readPosition_ != endPosition_)
	{
		if(*readPosition_ < 0x80)
		{
			out = *readPosition_;
			++readPosition_;
		}
		else
		{
			out = *readPosition_ & 0x7F;
			uint shift = 7;
			while(true)
			{
				if(++readPosition_ == endPosition_)
				{
					out = 0;
					isReadSuccessful_ = false;
					break;
				}
				uint byte = *readPosition_;
				if((byte & 0x80) == 0)
				{
					out |= byte << shift;
					++readPosition_;
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
		isReadSuccessful_ = false;
	}
	return out;
}

String ReadStream::readStr()
{
	String out;
	uint len = readNum();
	auto newPos = readPosition_ + len;
	if(newPos <= endPosition_)
	{
		auto str = readPosition_;
		readPosition_ = newPos;
		return String((const char*)str, len);
	}
	readPosition_ = endPosition_;
	isReadSuccessful_ = false;
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
