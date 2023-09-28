
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <cmath>

#include <opencv2/opencv.hpp>

using namespace std;


//////////////////////////////
/* 
 * struct of bytes for hash
 */
//////////////////////////////
template <std::size_t size>
struct big_int {
    std::array<std::uint8_t, size> bytes;

    static const unordered_map<char, uint8_t> hex_to_uint8;
    static const string uint8_to_hex;
    static const vector<uint8_t> count_bits_table;

    big_int() {}

    big_int(string s) {
        from_hex_string(s);
    }


    void from_hex_string(string inp) {
        // TODO: HERE should have length check and char range is 0-f
        if (inp.size() != size * 2) {
            cout << "hex string should have length of " 
                << size * 2 << ", but this string has length of " << inp.size() << endl;
        }
        size_t pos = 0;
        for (size_t i{0}; i < size; ++i) {
            // bytes[i] = hex_to_uint8[inp[pos++]] << 4;
            // bytes[i] += hex_to_uint8[inp[pos++]];
            bytes[i] = hex_to_uint8.at(inp[pos++]) << 4;
            bytes[i] += hex_to_uint8.at(inp[pos++]);
        }
    }

    string to_hex_string() const {
        string res;
        res.resize(size << 1);

        size_t pos = 0;
        for (size_t i{0}; i < size; ++i) {
            res[pos++] = uint8_to_hex[(bytes[i] & 0xf0) >> 4];
            res[pos++] = uint8_to_hex[bytes[i] & 0x0f];
        }
        return res;
    }

    void left_move_add_byte(uint8_t byte) {
        size_t n = size - 1;
        for (size_t i{0}; i < n; ++i) {
            bytes[i] = bytes[i+1];
        }
        bytes[n] = byte;
    }

    static uint16_t count_diff_bits(const big_int<size>& a, const big_int<size>& b) { 

        uint16_t res = 0;
        for (size_t i = 0; i < size; ++i) {
            uint8_t ind = a.bytes[i] ^ b.bytes[i];
            res += count_bits_table[ind];
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
};



template <std::size_t size>
const string big_int<size>::uint8_to_hex = "0123456789abcdef";

template <std::size_t size>
const unordered_map<char, uint8_t> big_int<size>::hex_to_uint8 = big_int<size>::init_hex_to_uint8();

template <std::size_t size>
const vector<uint8_t> big_int<size>::count_bits_table = big_int<size>::init_count_bits_table();

constexpr size_t n_bytes = 16;
// constexpr size_t n_bytes = 64;
using bignum_t = big_int<n_bytes>;

ostream& operator<<(ostream& os, bignum_t& bn) {
    os << bn.to_hex_string();
    return os;
}


////////////////////////////// big_int



using sample_t = pair<string, bignum_t>;

ostream& operator<<(ostream& os, sample_t& one) {
    os << one.first << "," << one.second;
    return os;
}


uint16_t thr = 10;
// uint16_t thr = 30;


bignum_t compute_dhash(string buf, uint16_t hw) {
    cv::Mat im = cv::imread(buf, cv::IMREAD_GRAYSCALE);
    if (im.empty()) cout << "empty image: " << buf << endl;

    cv::Mat resize;
    cv::resize(im, resize, cv::Size(hw + 1, hw + 1), 0, 0, cv::INTER_AREA);

    uint8_t *data = static_cast<uint8_t*>(resize.data);

    bignum_t hash;
    size_t byte_ind = 0;
    uint8_t byte = 0;
    uint8_t cnt = 0;
    uint16_t pos = 0;
    // traverse each row
    for (size_t h{0}; h < hw; ++h) {
        uint16_t ind1 = pos;
        uint16_t ind2 = ind1 + 1;

        for (size_t w{0}; w < hw; ++w) {
            byte = byte << 1;
            if (data[ind1] < data[ind2]) byte += 1;

            if (++cnt == 8) {
                hash.bytes[byte_ind++] = byte;
                cnt = 0;
                byte = 0;
            }
            ind1 += 1;
            ind2 += 1;
        }
        pos += hw + 1;
    }

    // traverse each column 
    byte = 0;
    cnt = 0;
    for (size_t w{0}; w < hw; ++w) {
        uint16_t ind1 = w;
        uint16_t ind2 = ind1 + hw + 1;

        for (size_t h{0}; h < hw; ++h) {
            byte = byte << 1;
            if (data[ind1] < data[ind2]) byte += 1;

            if (++cnt == 8) {
                hash.bytes[byte_ind++] = byte;
                cnt = 0;
                byte = 0;
            }
            ind1 += hw + 1;
            ind2 += hw + 1;
        }
    }

    return hash;
}


void gen_dhashes_worker(size_t tid, size_t n_proc,
        vector<string>& v_paths, vector<sample_t>& res, uint16_t hw) {

    size_t n_samples = v_paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        auto dhash = compute_dhash(v_paths[i], hw);
        res[i] = make_pair(v_paths[i], dhash);
    }
}


// compute dhash
vector<sample_t> gen_dhashes(string imgpths, size_t n_proc) {
    ifstream fin(imgpths, ios::in);
    stringstream ss;
    fin >> ss.rdbuf();
    fin.close();

    string buf;

    ss.clear();ss.seekg(0, ios::beg);
    size_t n_samples = 0;
    vector<string> v_paths;
    while (getline(ss, buf)) {
        v_paths.push_back(buf);
        ++n_samples;
    }


    uint16_t hw = std::sqrt(n_bytes * 8 / 2);
    if (hw % 8 != 0) {
        cout << "[warning]: num of bytes for hash should be divisible by 8" << endl;
    }

    vector<sample_t> res; res.resize(n_samples);
    
    vector<std::future<void>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, gen_dhashes_worker, 
                                  tid, n_proc, std::ref(v_paths), 
                                  std::ref(res), hw);
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid].wait();
    }

    return res;
}


// read generated hash
vector<sample_t> parse_hashes(string inpth) {
    ifstream fin(inpth, ios::in);
    stringstream ss;
    ss << fin.rdbuf();
    fin.close();

    string buf;

    ss.clear();ss.seekg(0, ios::beg);
    size_t n_samples = 0;
    while (getline(ss, buf)) {++n_samples;}

    vector<sample_t> res; res.reserve(n_samples);

    ss.clear();ss.seekg(0, ios::beg);
    while (getline(ss, buf)) {
        size_t pos = buf.find(",");
        string key = buf.substr(0, pos);
        string v = buf.substr(pos + 1);

        res.push_back(std::make_pair(key, bignum_t(v)));
    }
    return res;
}


// a triangle shape class, used to fetch coordinates from a global index
struct triangle_t {
private:
    int i{-1};
    int j{-1};

public:
    int get_i() {return i;}
    int get_j() {return j;}

    triangle_t(int pos) {
        int sum = 0;
        i = 0;
        while (true) {
            if (sum + i >= pos) break;
            sum += i + 1;
            ++i;
        }
        j = pos - sum;
    }

    void step(int n) { // move a step of n and update its coordinate
        while (j + n >= i + 1) {
            ++i;
            n -= i - j;
            j = 0;
        }
        j += n;
    }

    bool is_in_range(int n_levels) { // see if the current position is with a triagle with height n_levels
        if (i < 0 or j < 0) return false;
        if (i >= n_levels or j >= n_levels) return false;
        return true;
    }

    void print() { 
        cout << (*this) << endl;
    }

    friend ostream& operator<<(ostream& os, triangle_t& tr) {
        os << "(" << tr.i << ", " << tr.j << ")";
        return os;
    }
};


struct pair_t {
    size_t i;
    size_t j;
    uint16_t diff;
    pair_t(size_t i, size_t j, uint16_t diff): i(i), j(j), diff(diff) {}
    
    void print() {
        cout << (*this) << endl;
    }

    friend ostream& operator<<(ostream& os, pair_t& p) {
        os << "i=" << p.i << ", j=" << p.j << ", diff=" << p.diff;
        return os;
    }

    static vector<string> inds_to_strings_vector(const vector<sample_t>& samples, 
            const vector<pair_t>& pairs_ind) {
        vector<string> res;
        stringstream ss;
        for (auto &pair : pairs_ind) {
            ss.str(""); ss.clear();
            ss << samples[pair.i].first 
                << "," << samples[pair.j].first 
                << "," << pair.diff; 
            res.emplace_back(ss.str());
        }
        return res;
    }

};


vector<pair_t> down_triangle_worker(size_t tid, size_t n_proc, vector<sample_t>& samples) {
    size_t n_samples = samples.size();
    size_t res_size = n_samples * (n_samples - 1) / 2 / n_proc + 1;
    vector<pair_t> res; res.reserve(res_size);

    triangle_t coo = triangle_t(tid);
    while (coo.is_in_range(n_samples - 1)) {
        size_t i = coo.get_i() + 1;
        size_t j = coo.get_j();
        uint16_t diff = bignum_t::count_diff_bits(samples[i].second, samples[j].second);

        if (diff < thr) {
            res.push_back(pair_t(i, j, diff));
        }

        coo.step(n_proc);
    }

    return res;
}


vector<pair_t> get_dup_pairs_down_triangle(vector<sample_t>& samples, size_t n_proc) {

    vector<std::future<vector<pair_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async,
                down_triangle_worker, tid, n_proc, std::ref(samples));
    }

    vector<pair_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}


vector<size_t> remove_dups_from_pairs(const vector<pair_t>& dup_pairs) {
    unordered_map<size_t, unordered_set<size_t>> adj_table;
    for (auto& el : dup_pairs) {
        if (adj_table.find(el.i) == adj_table.end()) {
            adj_table[el.i] = unordered_set<size_t>();
        }
        if (adj_table.find(el.j) == adj_table.end()) {
            adj_table[el.j] = unordered_set<size_t>();
        }
        adj_table[el.i].insert(el.j);
        adj_table[el.j].insert(el.i);
    }

    size_t n_pairs = dup_pairs.size();
    vector<size_t> res; 
    res.reserve(n_pairs / 2 + 1);
    size_t max_val = 0;
    size_t max_ind = 0;
    for (auto& el : adj_table) {
        if (el.second.size() > max_val) {
            max_val = el.second.size();
            max_ind = el.first;
        }
    }
    while (max_val > 0) {
        size_t rm_ind = max_ind;
        res.push_back(rm_ind);

        adj_table.erase(rm_ind);
        max_val = 0;
        max_ind = -1;
        for (auto& el : adj_table) {
            if (el.second.find(rm_ind) != el.second.end()) {
                el.second.erase(rm_ind);
            }
            if (el.second.size() > max_val) {
                max_val = el.second.size();
                max_ind = el.first;
            }
        }
    }

    return res;
}


template<typename T>
vector<T> vector_remove_by_inds(vector<T>& samples, vector<size_t>& remove_inds) {
    size_t n_res = samples.size() - remove_inds.size();
    vector<T> res;
    res.reserve(n_res);

    unordered_set<size_t> us_remove_inds(remove_inds.begin(), remove_inds.end());

    size_t n_samples = samples.size();
    for (size_t i{0}; i < n_samples; ++i) {
        if (us_remove_inds.find(i) == us_remove_inds.end()) {
            res.push_back(samples[i]);
        }
    }
    return res;
}


vector<string> vector_remove_dhash(vector<sample_t>& samples) {
    size_t num = samples.size();
    vector<string> res; res.reserve(num);

    for (auto& el : samples) {
        res.push_back(el.first);
    }
    return res;
}


template<typename T>
void save_result(vector<T>& res, string savepth) {
    size_t n_samples = res.size();
    stringstream ss;
    for (size_t i{0}; i < n_samples; ++i) {
        ss << res[i];
        if (i < n_samples - 1) ss << endl;
    }

    ss.clear(); ss.seekg(0, ios::beg); 
    ofstream fout(savepth, ios::out);
    ss >> fout.rdbuf(); fout.close();
}


vector<size_t> rectangle_worker(size_t tid, size_t n_proc, 
                                vector<sample_t>& samples1, 
                                vector<sample_t>& samples2) {
    vector<size_t> res;
    size_t n_loop = samples1.size();
    size_t n_gallery = samples2.size();
    for (size_t i{tid}; i < n_loop; i += n_proc) {
        for (size_t j{0}; j < n_gallery; ++j) {
            uint16_t diff = bignum_t::count_diff_bits(samples1[i].second, samples2[j].second);
            if (diff < thr) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}


vector<size_t> get_dup_inds_rectangle(vector<sample_t>& samples1, 
                                       vector<sample_t>& samples2, size_t n_proc) {

    vector<std::future<vector<size_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, rectangle_worker, 
                                  tid, n_proc, std::ref(samples1), 
                                  std::ref(samples2));
    }

    vector<size_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}


/// dedup within one dataset
void dedup_one_dataset(string inpth, size_t n_proc) {

    cout << "generate dhash" << endl;
    auto samples = gen_dhashes(inpth, n_proc);
    save_result(samples, inpth + ".dhash");

    // cout << "read file" << endl;
    // vector<sample_t> samples = parse_hashes(inpth);
    // cout << "num of samples: " << samples.size() << endl;

    cout << "compare each pair" << endl;
    auto dup_pairs_ind = get_dup_pairs_down_triangle(samples, n_proc);
    auto dup_pairs_str = pair_t::inds_to_strings_vector(samples, dup_pairs_ind);
    save_result(dup_pairs_str, inpth + ".pair");

    cout << "search for to-be-removed" << endl;
    auto to_be_removed = remove_dups_from_pairs(dup_pairs_ind);

    cout << "remove inds and save" << endl;
    auto deduped = vector_remove_by_inds<sample_t>(samples, to_be_removed);
    save_result(deduped, inpth + ".dedup.dhash");
    auto res = vector_remove_dhash(deduped);
    save_result(res, inpth + ".dedup");

}


/// merge multi datasets together, each of which has already been deduplicated
void merge_datasets(vector<string> inpths, string savepth, size_t n_proc) {
    // TODO: assert inpths.size() >= 2
    
    vector<sample_t> deduped;
    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        cout << "read and merge " << inpths[i] << endl;
        auto samplesi = parse_hashes(inpths[i] + ".dedup.dhash");

        if (deduped.size() > 0) {
            auto dup_inds_src = get_dup_inds_rectangle(samplesi, deduped, n_proc);
            samplesi = vector_remove_by_inds<sample_t>(samplesi, dup_inds_src);
        }
        std::copy(samplesi.begin(), samplesi.end(), std::back_inserter(deduped));
    }

    auto res = vector_remove_dhash(deduped);
    save_result(res, savepth);
}


/* 
 *     === 
 *     === compile command:  
 *     === 
 *
 *         $ apt install libopencv-dev
 *         $ g++ -O2 main.cpp -std=c++14 -o main -lpthread $(pkg-config --libs --cflags opencv)
 *
 *     === 
 *     === void dedup_one_dataset(string inpth);
 *     === 
 *
 *         Steps of this function:
 *             1. read image paths from inpth, and compute dhash codes of each one, also write results to disk
 *             2. count bits difference of dhash code between each pair of images, and obtain the pairs with difference less than 10, these pairs are considered as duplicated pairs. Write pairs of image to disk (only image index in the input file). 
 *             3. remove images according to the pairs, those with most connections with others are removed first
 *             4. write the deduplicated image paths back to disk
 *
 *         Take `inpth="images.txt"` as example:
 *             input file `images.txt` format: 
 *                 /path/to/image1
 *                 /path/to/image2
 *                 /path/to/image3
 *                 ...
 *    
 *             generated `images.txt.dhash` format: 
 *                 /path/to/image1,9065d85ab39396e400ffff484833ff04
 *                 /path/to/image2,9065d85ab39396e400ffff484833ff04
 *                 /path/to/image3,9065d85ab39396e400ffff484833ff04
 *                 ...
 *    
 *             generated `images.txt.pair` format: 
 *                 i=72737, j=62320, diff=0
 *                 i=15063, j=9259, diff=1
 *                 i=75546, j=8643, diff=4
 *                 ...
 *    
 *             generated `images.txt.dedup` format: 
 *                 /path/to/image1
 *                 /path/to/image2
 *                 /path/to/image3
 *                 ...
 *
 */

/*         
 *     === 
 *     === void merge_datasets(vector<string> inpths, string savepth);
 *     === 
 *
 *         This takes in multi duplicated files, deduplicates and merges the contents together, then saves into the savepth
 *         This requires deduplication has already been carried out within each input file. It is better that each file is deduplicated first with the function of `dedup_one_dataset` separately, then use this function to merge. 
 *
 *         input file format:
 *             /path/to/image1
 *             /path/to/image2
 *             /path/to/image3
 *             ...
 *
 *         output file format: 
 *             /path/to/image1
 *             /path/to/image2
 *             /path/to/image3
 *             ...
 *
 */

int main(int argc, char* argv[]) {

    string cmd(argv[1]);
    size_t n_proc = static_cast<size_t>(std::stoi(string(argv[2])));

    if (cmd == "one_dataset") {

        string inpth(argv[3]);
        dedup_one_dataset(inpth, n_proc);

    } else if (cmd == "merge") {

        vector<string> inpths;
        for (int i{3}; i < argc - 1; ++i) {
            inpths.push_back(argv[i]);
        }
        string savepth(argv[argc - 1]);
        merge_datasets(inpths, savepth, n_proc);
    }

    return 0;
}

