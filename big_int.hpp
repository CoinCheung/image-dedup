#ifndef _BIG_INT_H_
#define _BIG_INT_H_


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <bit>
#include <string>
#include <cstring>


//////////////////////////////
/* 
 * struct of bytes for hash
 */
//////////////////////////////

using namespace std;


template <std::size_t size>
struct big_int {
    std::array<std::uint8_t, size> m_bytes;

    static const unordered_map<char, uint8_t> hex_to_uint8;
    static const string uint8_to_hex;
    static const vector<uint8_t> count_bits_table;

    big_int() {}

    big_int(const big_int<size>& b): m_bytes(b.m_bytes) {}

    static big_int<size> create_from_string_hex(const string& s) {
        big_int<size> res{};
        res.from_hex_string(s);
        return res;
    }

    void from_hex_string(string inp) {
        // TODO: HERE should have length check and char range is 0-f
        if (inp.size() != size * 2) {
            cerr << "[ERROR] hex string should have length of " 
                << size * 2 << ", but this string has length of " << inp.size() 
                << ": " << inp << endl;
        }
        size_t pos = 0;
        for (size_t i{0}; i < size; ++i) {
            // m_bytes[i] = hex_to_uint8[inp[pos++]] << 4;
            // m_bytes[i] += hex_to_uint8[inp[pos++]];
            m_bytes[i] = hex_to_uint8.at(inp[pos++]) << 4;
            m_bytes[i] += hex_to_uint8.at(inp[pos++]);
        }
    }

    string to_hex_string() const {
        string res;
        res.resize(size << 1);

        size_t pos = 0;
        for (size_t i{0}; i < size; ++i) {
            res[pos++] = uint8_to_hex[(m_bytes[i] & 0xf0) >> 4];
            res[pos++] = uint8_to_hex[m_bytes[i] & 0x0f];
        }
        return res;
    }

    void left_move_add_byte(uint8_t byte) {
        size_t n = size - 1;
        for (size_t i{0}; i < n; ++i) {
            m_bytes[i] = m_bytes[i+1];
        }
        m_bytes[n] = byte;
    }

    static uint16_t count_diff_bits(const big_int<size>& a, const big_int<size>& b) { 
        return count_diff_bits_v3(a, b);
    }

    static uint16_t count_diff_bits_v1(const big_int<size>& a, const big_int<size>& b) { 

        uint16_t res = 0;
        for (size_t i = 0; i < size; ++i) {
            uint8_t ind = a.m_bytes[i] ^ b.m_bytes[i];
            res += count_bits_table[ind];
        }
        return res;
    }

    static uint16_t count_diff_bits_v2(const big_int<size>& a, const big_int<size>& b) { 
        uint16_t res = 0;
        size_t n_rem = size;
        while (n_rem > sizeof(uint64_t)) {
            size_t pos = size - n_rem;
            uint64_t v_a, v_b;
            std::memcpy(&v_a, &a[pos], sizeof(uint64_t));
            std::memcpy(&v_b, &b[pos], sizeof(uint64_t));
            res += bitset<sizeof(uint64_t)>{v_a ^ v_b}.count();
            n_rem -= sizeof(uint64_t);
        }
        return res;
    }

    static uint16_t count_diff_bits_v3(const big_int<size>& a, const big_int<size>& b) { 
        // we do not use reinterpret_cast because it is undefined behavior, and is not recommended
        // std::memcpy is best here, compile will not really copy memory at all. No performance overhead

        uint16_t res = 0;
        size_t n_rem = size;
        while (n_rem > sizeof(uint64_t)) {
            size_t pos = size - n_rem;
            uint64_t v_a, v_b;
            std::memcpy(&v_a, &a[pos], sizeof(uint64_t));
            std::memcpy(&v_b, &b[pos], sizeof(uint64_t));
            res += std::popcount(v_a ^ v_b);
            n_rem -= sizeof(uint64_t);
        }
        // for remainings
        if (n_rem > sizeof(uint32_t)) {
            size_t pos = size - n_rem;
            uint32_t v_a, v_b;
            std::memcpy(&v_a, &a[pos], sizeof(uint32_t));
            std::memcpy(&v_b, &b[pos], sizeof(uint32_t));
            res += std::popcount(v_a ^ v_b);
            n_rem -= sizeof(uint32_t);
        }
        while (n_rem > 0) {
            size_t pos = size - n_rem;
            uint8_t ind = a.m_bytes[pos] ^ b.m_bytes[pos];
            res += count_bits_table[ind];
            --n_rem;
        }
        return res;
    }


    //// functions to initialize member variables
    static unordered_map<char, uint8_t> init_hex_to_uint8() {
        unordered_map<char, uint8_t> res;
        for (size_t i{0}; i < 10; ++i) {
            char hex = '0' + i;
            res[hex] = i;
        }
        for (size_t i{10}; i < 16; ++i) {
            char hex = 'a' + i - 10;
            res[hex] = i;
        }
        return res;
    }

    static vector<uint8_t> init_count_bits_table() {
        vector<uint8_t> res;
        for (size_t i = 0; i <= 255; ++i) {
            uint16_t cnt = 0;
            uint8_t ii = static_cast<uint8_t>(i);
            while (ii > 0) {
                cnt += ii & 0x01;
                ii = ii >> 1;
            }
            res.push_back(cnt);
        }
        return res;
    }

    uint8_t& operator[] (size_t ind) {
        return m_bytes[ind];
    }

    const uint8_t& operator[] (size_t ind) const {
        return m_bytes[ind];
    }

    bool operator==(const big_int<size>& o) const {
        // for (size_t i{0}; i < size; ++i) {
        //     if (o[i] != m_bytes[i]) return false;
        // }

        size_t n_rem = size;
        while (n_rem > sizeof(uint64_t)) {
            size_t pos = size - n_rem;
            uint64_t v_a, v_b;
            std::memcpy(&v_a, &o[pos], sizeof(uint64_t));
            std::memcpy(&v_b, &m_bytes[pos], sizeof(uint64_t));
            if (v_a != v_b) return false;
            n_rem -= sizeof(uint64_t);
        }
        // for remainings
        if (n_rem > sizeof(uint32_t)) {
            size_t pos = size - n_rem;
            uint32_t v_a, v_b;
            std::memcpy(&v_a, &o[pos], sizeof(uint32_t));
            std::memcpy(&v_b, &m_bytes[pos], sizeof(uint32_t));
            if (v_a != v_b) return false;
            n_rem -= sizeof(uint32_t);
        }
        while (n_rem > 0) {
            size_t pos = size - n_rem;
            if (o.m_bytes[pos] != m_bytes[pos]) return false;
            --n_rem;
        }
        return true;


        // uint16_t n_chunk = size >> 3;
        // const uint64_t *p1 = reinterpret_cast<const uint64_t*>(&o[0]);
        // const uint64_t *p2 = reinterpret_cast<const uint64_t*>(&m_bytes[0]);
        // for (size_t i{0}; i < n_chunk; ++i) {
        //     if ((*p1) != (*p2)) return false;
        //     ++p1; ++p2;
        // }
        // return true;
    }

};

template <std::size_t size>
const string big_int<size>::uint8_to_hex = "0123456789abcdef";

template <std::size_t size>
const unordered_map<char, uint8_t> big_int<size>::hex_to_uint8 = big_int<size>::init_hex_to_uint8();

template <std::size_t size>
const vector<uint8_t> big_int<size>::count_bits_table = big_int<size>::init_count_bits_table();


template <std::size_t size>
ostream& operator<<(ostream& os, const big_int<size>& bn) {
    os << bn.to_hex_string();
    return os;
}


//// functions for supporting map/unordered_map key
namespace std {
// for map
template <std::size_t size>
bool operator<(const big_int<size>& bn1, const big_int<size>& bn2) {
    for (size_t i{0}; i < size; ++i) {
        if (bn1[i] == bn2[i]) continue;
        return bn1[i] < bn2[i];
    }
    return false;
}

// for unordered_map

template<size_t size>
struct hash<big_int<size>> {
    size_t operator()(const big_int<size>& o) const noexcept {
        return std::hash<string>{}(o.to_hex_string());
    }
};

}

#endif
