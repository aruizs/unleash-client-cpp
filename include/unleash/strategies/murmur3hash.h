#ifndef UNLEASH_MURMUR3HASH_H
#define UNLEASH_MURMUR3HASH_H
//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <string>

namespace unleash {

void murmurHash3X8632(const void *key, int len, uint32_t seed, void *out);

uint32_t normalizedMurmur3(const std::string &key, uint32_t seed = 0);


}  // namespace unleash

#endif  //UNLEASH_MURMUR3HASH_H
