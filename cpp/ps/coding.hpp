#pragma once
#include <deque>
#include <boost/utility.hpp>
#include "succinct/util.hpp"

namespace ps {
namespace coding {

inline uint32_t decode_fixed_32(const char* ptr)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    // Load the raw bytes
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
#else
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
        | (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
#endif
}

inline uint64_t decode_fixed_64(const char* ptr)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    // Load the raw bytes
    uint64_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
#else
    uint64_t lo = decode_fixed_32(ptr);
    uint64_t hi = decode_fixed_32(ptr + 4);
    return (hi << 32) | lo;
#endif
}

inline void encode_fixed_64(char* buf, uint64_t value)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  memcpy(buf, &value, sizeof(value));
#else
  buf[0] = value & 0xff;
  buf[1] = (value >> 8) & 0xff;
  buf[2] = (value >> 16) & 0xff;
  buf[3] = (value >> 24) & 0xff;
  buf[4] = (value >> 32) & 0xff;
  buf[5] = (value >> 40) & 0xff;
  buf[6] = (value >> 48) & 0xff;
  buf[7] = (value >> 56) & 0xff;
#endif
}

inline void put_fixed_64(std::string& dst, uint64_t value)
{
  char buf[sizeof(value)];
  encode_fixed_64(buf, value);
  dst.append(buf, sizeof(buf));
}

}
}