#include <Core/ByteStream.h>
#include <Core/Utils.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

namespace Vortex {

// ================================================================================================
// WriteStream.

WriteStream::WriteStream() {
    buffer_ = (uint8_t*)malloc(128);
    current_size_ = 0;
    capacity_ = 128;
    is_external_buffer_ = false;
    is_write_successful_ = true;
}

WriteStream::WriteStream(void* out, int bytes) {
    buffer_ = (uint8_t*)out;
    current_size_ = 0;
    capacity_ = bytes;
    is_external_buffer_ = true;
    is_write_successful_ = true;
}

WriteStream::~WriteStream() {
    if (!is_external_buffer_) free(buffer_);
}

void WriteStream::write(const void* in, int bytes) {
    int newSize = current_size_ + bytes;
    if (newSize <= capacity_) {
        memcpy(buffer_ + current_size_, in, bytes);
        current_size_ = newSize;
    } else if (!is_external_buffer_) {
        capacity_ = max(capacity_ << 1, newSize);
        buffer_ = (uint8_t*)realloc(buffer_, capacity_);
        memcpy(buffer_ + current_size_, in, bytes);
        current_size_ = newSize;
    } else {
        is_write_successful_ = false;
    }
}

void WriteStream::write8(const void* val) {
    if (current_size_ < capacity_) {
        buffer_[current_size_] = *(const uint8_t*)val;
        ++current_size_;
    } else if (!is_external_buffer_) {
        capacity_ <<= 1;
        buffer_ = (uint8_t*)realloc(buffer_, capacity_);
        buffer_[current_size_] = *(const uint8_t*)val;
        ++current_size_;
    } else {
        is_write_successful_ = false;
    }
}

void WriteStream::write16(const void* val) {
    int newSize = current_size_ + sizeof(uint16_t);
    if (newSize <= capacity_) {
        *(uint16_t*)(buffer_ + current_size_) = *(const uint16_t*)val;
        current_size_ = newSize;
    } else if (!is_external_buffer_) {
        capacity_ <<= 1;
        buffer_ = (uint8_t*)realloc(buffer_, capacity_);
        *(uint16_t*)(buffer_ + current_size_) = *(const uint16_t*)val;
        current_size_ = newSize;
    } else {
        is_write_successful_ = false;
    }
}

void WriteStream::write32(const void* val) {
    int newSize = current_size_ + sizeof(uint32_t);
    if (newSize <= capacity_) {
        *(uint32_t*)(buffer_ + current_size_) = *(const uint32_t*)val;
        current_size_ = newSize;
    } else if (!is_external_buffer_) {
        capacity_ <<= 1;
        buffer_ = (uint8_t*)realloc(buffer_, capacity_);
        *(uint32_t*)(buffer_ + current_size_) = *(const uint32_t*)val;
        current_size_ = newSize;
    } else {
        is_write_successful_ = false;
    }
}

void WriteStream::write64(const void* val) {
    int newSize = current_size_ + sizeof(uint64_t);
    if (newSize <= capacity_) {
        *(uint64_t*)(buffer_ + current_size_) = *(const uint64_t*)val;
        current_size_ = newSize;
    } else if (!is_external_buffer_) {
        capacity_ <<= 1;
        buffer_ = (uint8_t*)realloc(buffer_, capacity_);
        *(uint64_t*)(buffer_ + current_size_) = *(const uint64_t*)val;
        current_size_ = newSize;
    } else {
        is_write_successful_ = false;
    }
}

void WriteStream::writeNum(uint32_t num) {
    if (num < 0x80) {
        write8(&num);
    } else {
        uint8_t buf[8];
        uint32_t index = 0;
        while (num >= 0x80) {
            buf[index] = (num & 0x7F) | 0x80;
            num >>= 7;
            ++index;
        }
        buf[index] = num;
        write(buf, index + 1);
    }
}

void WriteStream::writeStr(const std::string& str) {
    writeNum(str.length());
    write(str.data(), str.length());
}

// ================================================================================================
// ReadStream.

ReadStream::ReadStream(const void* in, int bytes) {
    read_position_ = (const uint8_t*)in;
    end_position_ = read_position_ + bytes;
    is_read_successful_ = true;
}

ReadStream::~ReadStream() {}

void ReadStream::skip(int bytes) {
    if (read_position_ + bytes <= end_position_) {
        read_position_ += bytes;
    } else {
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

void ReadStream::read(void* out, int bytes) {
    if (read_position_ + bytes <= end_position_) {
        memcpy(out, read_position_, bytes);
        read_position_ += bytes;
    } else {
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

void ReadStream::read8(void* out) {
    if (read_position_ != end_position_) {
        *(uint8_t*)out = *read_position_;
        ++read_position_;
    } else {
        *(uint8_t*)out = 0;
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

void ReadStream::read16(void* out) {
    auto newPos = read_position_ + sizeof(uint16_t);
    if (newPos <= end_position_) {
        *(uint16_t*)out = *(const uint16_t*)read_position_;
        read_position_ = newPos;
    } else {
        *(uint16_t*)out = 0;
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

void ReadStream::read32(void* out) {
    auto newPos = read_position_ + sizeof(uint32_t);
    if (newPos <= end_position_) {
        *(uint32_t*)out = *(const uint32_t*)read_position_;
        read_position_ = newPos;
    } else {
        *(uint32_t*)out = 0;
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

void ReadStream::read64(void* out) {
    auto newPos = read_position_ + sizeof(uint64_t);
    if (newPos <= end_position_) {
        *(uint64_t*)out = *(const uint64_t*)read_position_;
        read_position_ = newPos;
    } else {
        *(uint64_t*)out = 0;
        read_position_ = end_position_;
        is_read_successful_ = false;
    }
}

uint32_t ReadStream::readNum() {
    uint32_t out;
    if (read_position_ != end_position_) {
        if (*read_position_ < 0x80) {
            out = *read_position_;
            ++read_position_;
        } else {
            out = *read_position_ & 0x7F;
            uint32_t shift = 7;
            while (true) {
                if (++read_position_ == end_position_) {
                    out = 0;
                    is_read_successful_ = false;
                    break;
                }
                uint32_t byte = *read_position_;
                if ((byte & 0x80) == 0) {
                    out |= byte << shift;
                    ++read_position_;
                    break;
                }
                out |= (byte & 0x7F) << shift;
                shift += 7;
            }
        }
    } else {
        out = 0;
        is_read_successful_ = false;
    }
    return out;
}

std::string ReadStream::readStr() {
    std::string out;
    uint32_t len = readNum();
    auto newPos = read_position_ + len;
    if (newPos <= end_position_) {
        auto str = read_position_;
        read_position_ = newPos;
        return std::string((const char*)str, len);
    }
    read_position_ = end_position_;
    is_read_successful_ = false;
    return std::string();
}

void ReadStream::readNum(uint32_t& num) { num = readNum(); }

void ReadStream::readStr(std::string& str) { str = readStr(); }

};  // namespace Vortex
