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
	myBuf = (uchar*)malloc(128);
	mySize = 0;
	myCap = 128;
	myExtern = false;
	mySuccess = true;
}

WriteStream::WriteStream(void* out, int bytes)
{
	myBuf = (uchar*)out;
	mySize = 0;
	myCap = bytes;
	myExtern = true;
	mySuccess = true;
}

WriteStream::~WriteStream()
{
	if(!myExtern) free(myBuf);
}

void WriteStream::write(const void* in, int bytes)
{
	int newSize = mySize + bytes;
	if(newSize <= myCap)
	{
		memcpy(myBuf + mySize, in, bytes);
		mySize = newSize;
	}
	else if(!myExtern)
	{
		myCap = max(myCap << 1, newSize);
		myBuf = (uchar*)realloc(myBuf, myCap);
		memcpy(myBuf + mySize, in, bytes);
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void WriteStream::write8(const void* val)
{
	if(mySize < myCap)
	{
		myBuf[mySize] = *(const uint8_t*)val;
		++mySize;
	}
	else if(!myExtern)
	{
		myCap <<= 1;
		myBuf = (uchar*)realloc(myBuf, myCap);
		myBuf[mySize] = *(const uint8_t*)val;
		++mySize;
	}
	else
	{
		mySuccess = false;
	}
}

void WriteStream::write16(const void* val)
{
	int newSize = mySize + sizeof(uint16_t);
	if(newSize <= myCap)
	{
		*(uint16_t*)(myBuf + mySize) = *(const uint16_t*)val;
		mySize = newSize;
	}
	else if(!myExtern)
	{
		myCap <<= 1;
		myBuf = (uchar*)realloc(myBuf, myCap);
		*(uint16_t*)(myBuf + mySize) = *(const uint16_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void WriteStream::write32(const void* val)
{
	int newSize = mySize + sizeof(uint32_t);
	if(newSize <= myCap)
	{
		*(uint32_t*)(myBuf + mySize) = *(const uint32_t*)val;
		mySize = newSize;
	}
	else if(!myExtern)
	{
		myCap <<= 1;
		myBuf = (uchar*)realloc(myBuf, myCap);
		*(uint32_t*)(myBuf + mySize) = *(const uint32_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void WriteStream::write64(const void* val)
{
	int newSize = mySize + sizeof(uint64_t);
	if(newSize <= myCap)
	{
		*(uint64_t*)(myBuf + mySize) = *(const uint64_t*)val;
		mySize = newSize;
	}
	else if(!myExtern)
	{
		myCap <<= 1;
		myBuf = (uchar*)realloc(myBuf, myCap);
		*(uint64_t*)(myBuf + mySize) = *(const uint64_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
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
	myPos = (const uchar*)in;
	myEnd = myPos + bytes;
	mySuccess = true;
}

ReadStream::~ReadStream()
{
}

void ReadStream::skip(int bytes)
{
	if(myPos + bytes <= myEnd)
	{
		myPos += bytes;
	}
	else
	{
		myPos = myEnd;
		mySuccess = false;
	}
}

void ReadStream::read(void* out, int bytes)
{
	if(myPos + bytes <= myEnd)
	{
		memcpy(out, myPos, bytes);
		myPos += bytes;
	}
	else
	{
		myPos = myEnd;
		mySuccess = false;
	}
}

void ReadStream::read8(void* out)
{
	if(myPos != myEnd)
	{
		*(uint8_t*)out = *myPos;
		++myPos;
	}
	else
	{
		*(uint8_t*)out = 0;
		myPos = myEnd;
		mySuccess = false;
	}
}

void ReadStream::read16(void* out)
{
	auto newPos = myPos + sizeof(uint16_t);
	if(newPos <= myEnd)
	{
		*(uint16_t*)out = *(const uint16_t*)myPos;
		myPos = newPos;
	}
	else
	{
		*(uint16_t*)out = 0;
		myPos = myEnd;
		mySuccess = false;
	}
}


void ReadStream::read32(void* out)
{
	auto newPos = myPos + sizeof(uint32_t);
	if(newPos <= myEnd)
	{
		*(uint32_t*)out = *(const uint32_t*)myPos;
		myPos = newPos;
	}
	else
	{
		*(uint32_t*)out = 0;
		myPos = myEnd;
		mySuccess = false;
	}
}

void ReadStream::read64(void* out)
{
	auto newPos = myPos + sizeof(uint64_t);
	if(newPos <= myEnd)
	{
		*(uint64_t*)out = *(const uint64_t*)myPos;
		myPos = newPos;
	}
	else
	{
		*(uint64_t*)out = 0;
		myPos = myEnd;
		mySuccess = false;
	}
}

uint ReadStream::readNum()
{
	uint32_t out;
	if(myPos != myEnd)
	{
		if(*myPos < 0x80)
		{
			out = *myPos;
			++myPos;
		}
		else
		{
			out = *myPos & 0x7F;
			uint shift = 7;
			while(true)
			{
				if(++myPos == myEnd)
				{
					out = 0;
					mySuccess = false;
					break;
				}
				uint byte = *myPos;
				if((byte & 0x80) == 0)
				{
					out |= byte << shift;
					++myPos;
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
		mySuccess = false;
	}
	return out;
}

String ReadStream::readStr()
{
	String out;
	uint len = readNum();
	auto newPos = myPos + len;
	if(newPos <= myEnd)
	{
		auto str = myPos;
		myPos = newPos;
		return String((const char*)str, len);
	}
	myPos = myEnd;
	mySuccess = false;
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
