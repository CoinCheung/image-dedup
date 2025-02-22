
#ifndef _HASH_FUNC_H_
#define _HASH_FUNC_H_

#include <cstdint>
#include <string>
#include "samples.hpp"

using namespace std;


md5_t compute_md5(const vector<char>&);

md5_t compute_md5(const string&);

dhash_t compute_dhash(const string&, const uint16_t);

binhash_t compute_binbash(const string&);


#endif
