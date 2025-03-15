
#ifndef _HASH_FUNC_H_
#define _HASH_FUNC_H_

#include <cstdint>
#include <string>
#include "big_int.hpp"

using namespace std;


constexpr size_t nbytes_dhash = 256; // resize image to 32x32
constexpr size_t nbytes_phash = 32; // resize image to 64x64
                                
using dhash_t = big_int<nbytes_dhash>;
using phash_t = big_int<nbytes_phash>;
using md5_t = big_int<16>;
using binhash_t = big_int<sizeof(size_t)>;


md5_t compute_md5(const vector<char>&);

md5_t compute_md5(const string&);

dhash_t compute_dhash(const string&, const uint16_t);

phash_t compute_phash(const string&, const uint16_t);

binhash_t compute_binbash(const string&);


#endif
