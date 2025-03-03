#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Common/Serialize.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Serializer.

Serializer::Serializer()
	: myData((uchar*)malloc(128))
	, mySize(0)
	, myCap(128)
	, myIsOwner(true)
	, mySuccess(true)
{
}

Serializer::Serializer(void* out, size_t bytes)
	: myData((uchar*)out)
	, mySize(0)
	, myCap(bytes)
	, myIsOwner(false)
	, mySuccess(true)
{
}

Serializer::~Serializer()
{
	if (myIsOwner) free(myData);
}

void Serializer::skip(size_t bytes)
{
	auto newSize = mySize + bytes;
	if (newSize <= myCap)
	{
		mySize = newSize;
	}
	else if (myIsOwner)
	{
		myCap = max(myCap << 1, newSize);
		myData = (uchar*)realloc(myData, myCap);
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::write(const void* in, size_t bytes)
{
	auto newSize = mySize + bytes;
	if (newSize <= myCap)
	{
		memcpy(myData + mySize, in, bytes);
		mySize = newSize;
	}
	else if (myIsOwner)
	{
		myCap = max(myCap << 1, newSize);
		myData = (uchar*)realloc(myData, myCap);
		memcpy(myData + mySize, in, bytes);
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::write8(const void* val)
{
	if (mySize < myCap)
	{
		myData[mySize] = *(const uint8_t*)val;
		++mySize;
	}
	else if (myIsOwner)
	{
		myCap <<= 1;
		myData = (uchar*)realloc(myData, myCap);
		myData[mySize] = *(const uint8_t*)val;
		++mySize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::write16(const void* val)
{
	auto newSize = mySize + sizeof(uint16_t);
	if (newSize <= myCap)
	{
		*(uint16_t*)(myData + mySize) = *(const uint16_t*)val;
		mySize = newSize;
	}
	else if (myIsOwner)
	{
		myCap <<= 1;
		myData = (uchar*)realloc(myData, myCap);
		*(uint16_t*)(myData + mySize) = *(const uint16_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::write32(const void* val)
{
	auto newSize = mySize + sizeof(uint32_t);
	if (newSize <= myCap)
	{
		*(uint32_t*)(myData + mySize) = *(const uint32_t*)val;
		mySize = newSize;
	}
	else if (myIsOwner)
	{
		myCap <<= 1;
		myData = (uchar*)realloc(myData, myCap);
		*(uint32_t*)(myData + mySize) = *(const uint32_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::write64(const void* val)
{
	auto newSize = mySize + sizeof(uint64_t);
	if (newSize <= myCap)
	{
		*(uint64_t*)(myData + mySize) = *(const uint64_t*)val;
		mySize = newSize;
	}
	else if (myIsOwner)
	{
		myCap <<= 1;
		myData = (uchar*)realloc(myData, myCap);
		*(uint64_t*)(myData + mySize) = *(const uint64_t*)val;
		mySize = newSize;
	}
	else
	{
		mySuccess = false;
	}
}

void Serializer::writeNum(uint64_t num)
{
	if (num < 0x80)
	{
		write8(&num);
	}
	else
	{
		uchar buf[8];
		size_t index = 0;
		while (num >= 0x80)
		{
			buf[index] = (num & 0x7F) | 0x80;
			num >>= 7;
			++index;
		}
		buf[index] = (uchar)num;
		write(buf, index + 1);
	}
}

void Serializer::writeStr(stringref str)
{
	writeNum((uint)str.length());
	write(str.data(), str.length());
}

// =====================================================================================================================
// Deserializer.

Deserializer::Deserializer(const void* in)
	: myPos((const uchar*)in)
	, myEnd((const uchar*)-1)
	, mySuccess(true)
{
}

Deserializer::Deserializer(const void* in, size_t bytes)
	: myPos((const uchar*)in)
	, myEnd((const uchar*)in + bytes)
	, mySuccess(true)
{
}

Deserializer::~Deserializer()
{
}

void Deserializer::skip(size_t bytes)
{
	if (myPos + bytes <= myEnd)
	{
		myPos += bytes;
	}
	else
	{
		myPos = myEnd;
		mySuccess = false;
	}
}

void Deserializer::read(void* out, size_t bytes)
{
	if (myPos + bytes <= myEnd)
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

void Deserializer::read8(void* out)
{
	if (myPos != myEnd)
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

void Deserializer::read16(void* out)
{
	auto newPos = myPos + sizeof(uint16_t);
	if (newPos <= myEnd)
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

void Deserializer::read32(void* out)
{
	auto newPos = myPos + sizeof(uint32_t);
	if (newPos <= myEnd)
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

void Deserializer::read64(void* out)
{
	auto newPos = myPos + sizeof(uint64_t);
	if (newPos <= myEnd)
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

uint Deserializer::readNum()
{
	uint32_t out;
	if (myPos != myEnd)
	{
		if (*myPos < 0x80)
		{
			out = *myPos;
			++myPos;
		}
		else
		{
			out = *myPos & 0x7F;
			uint shift = 7;
			while (true)
			{
				if (++myPos == myEnd)
				{
					out = 0;
					mySuccess = false;
					break;
				}
				uint byte = *myPos;
				if ((byte & 0x80) == 0)
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

string Deserializer::readStr()
{
	string out;
	uint len = readNum();
	auto newPos = myPos + len;
	if (newPos <= myEnd)
	{
		auto str = myPos;
		myPos = newPos;
		return string((const char*)str, len);
	}
	myPos = myEnd;
	mySuccess = false;
	return string();
}

void Deserializer::readNum(uint& num)
{
	num = readNum();
}

void Deserializer::readStr(string& str)
{
	str = readStr();
}

} // namespace AV