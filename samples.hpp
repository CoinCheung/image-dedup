
#ifndef _SAMPLES_H_
#define _SAMPLES_H_

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "helper.h"
#include "big_int.hpp"


/// hash types
// constexpr size_t n_bytes = 16;
// constexpr size_t n_bytes = 64;

constexpr size_t n_bytes = 256;
using dhash_t = big_int<n_bytes>;
using md5_t = big_int<16>;
using binhash_t = big_int<sizeof(size_t)>;

//////////////////////////////
///
/* 
 * struct of sample_t
 */
//////////////////////////////

using namespace std;



template<typename hash_t>
struct sample_t {
public:
    string key;
    hash_t hash;

    sample_t() {}
    sample_t(string key, hash_t hash): key(key), hash(hash) {}

    void set(const string& k, const hash_t& h) {
        key = k; hash = h;
    }

    bool operator==(const sample_t<hash_t>& o) const {
        return hash == o.hash;
    }

    string to_string() const {
        return key + "," + hash.to_hex_string();
    }
};


template<typename hash_t>
ostream& operator<<(ostream& os, const sample_t<hash_t>& one) {
    os << one.to_string();
    return os;
}

//// functions for supporting map/unordered_map key
namespace std {
// for map
template <typename hash_t>
bool operator<(const sample_t<hash_t>& s1, const sample_t<hash_t>& s2) {
    return (s1.hash) < (s2.hash);
}

// for unordered_map

template <typename hash_t>
struct hash<sample_t<hash_t>> {
    size_t operator()(const sample_t<hash_t>& o) const noexcept {
        return std::hash<hash_t>{}(o.hash);
    }
};

}


//////////////////////////////
///
/* 
 * struct of sample_set
 */
//////////////////////////////

template <typename hash_t>
struct sample_set {
public:
    vector<sample_t<hash_t>> samples;

    sample_set() {}

    sample_set(const string inpth) {
        load_samples(inpth);
    }

    void reserve(const size_t size) {
        samples.reserve(size);
    }

    void resize(const size_t size) {
        samples.resize(size);
    }

    size_t size() {
        return samples.size();
    }

    void emplace_back(const string& key, const hash_t& code) {
        samples.emplace_back(key, code);
    }

    sample_t<hash_t>& operator[](const size_t ind) {
        return samples[ind];
    }

    const sample_t<hash_t>& operator[](const size_t ind) const {
        return samples[ind];
    }

    void load_samples(const string inpth) {

        cout << "load from: " << inpth << endl;

        auto lines = read_lines(inpth);
        size_t n_samples = lines.size();

        samples.reserve(n_samples);

        for (string& l : lines) {
            size_t pos = l.find(",");
            string key = l.substr(0, pos);
            string v = l.substr(pos + 1);

            samples.emplace_back(key, hash_t(v));
        }
    }

    void save_samples(const string savepth) {
        size_t n_samples = samples.size();
        stringstream ss;
        for (size_t i{0}; i < n_samples - 1; ++i) {
            ss << samples[i] << endl;
        }
        ss << samples[n_samples - 1];

        ss.clear(); ss.seekg(0, ios::beg); 
        ofstream fout(savepth, ios::out);
        if (!fout.is_open()) {
            cerr << "[ERROR] open for write fail: " << savepth 
                << ", errno: " << errno << endl;
        }
        ss >> fout.rdbuf(); fout.close();
    }

    void save_keys(const string savepth) {
        size_t n_samples = samples.size();
        stringstream ss;
        for (size_t i{0}; i < n_samples - 1; ++i) {
            ss << samples[i].key << endl;
        }
        ss << samples[n_samples - 1].key;

        ss.clear(); ss.seekg(0, ios::beg); 
        ofstream fout(savepth, ios::out);
        if (!fout.is_open()) {
            cerr << "[ERROR] open for write fail: " << savepth 
                << ", errno: " << errno << endl;
        }
        ss >> fout.rdbuf(); fout.close();
    }

    void dedup_by_identical_hash() {
        unordered_set<sample_t<hash_t>> hash(
                samples.begin(), samples.end());
        samples.resize(0);
        samples.assign(hash.begin(), hash.end());
    }

    void remove_by_given_inds(const vector<size_t>& inds) {
        unordered_set<size_t> to_removed_inds(inds.begin(), inds.end());
        size_t n_res = samples.size() - inds.size();
        size_t n_samples = samples.size();
        vector<sample_t<hash_t>> res; res.reserve(n_res);

        for (size_t i{0}; i < n_samples; ++i) {
            if (to_removed_inds.find(i) == to_removed_inds.end()) {
                res.push_back(samples[i]);
            }
        }

        samples.swap(res);
    }

    void merge_other(const sample_set<hash_t>& other) {
        std::copy(other.samples.begin(), other.samples.end(), 
                std::back_inserter(samples));
    }

};


#endif

