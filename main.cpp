
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <algorithm>
#include <array>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <cmath>

#include <opencv2/opencv.hpp>
#include <openssl/evp.h>

using namespace std;


// constexpr size_t n_bytes = 16;
// constexpr size_t n_bytes = 64;
constexpr size_t n_bytes = 256;
// uint16_t thr = 10;
// uint16_t thr = 30;
uint16_t thr = 200;


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
            cerr << "[ERROR] hex string should have length of " 
                << size * 2 << ", but this string has length of " << inp.size() 
                << ": " << inp << endl;
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

        // uint16_t res = 0;
        // for (size_t i = 0; i < size; ++i) {
        //     uint8_t ind = a.bytes[i] ^ b.bytes[i];
        //     res += count_bits_table[ind];
        // }
        // return res;

        uint16_t res = 0;
        uint16_t n_chunk = size >> 3;
        auto *p1 = reinterpret_cast<const uint64_t*>(&a[0]);
        auto *p2 = reinterpret_cast<const uint64_t*>(&b[0]);

        for (size_t i{0}; i < n_chunk; ++i) {
            res += bitset<64>{(*p1) ^ (*p2)}.count();
            ++p1; ++p2;
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
        return bytes[ind];
    }

    const uint8_t& operator[] (size_t ind) const {
        return bytes[ind];
    }

    bool operator==(const big_int<size>& o) const {
        // for (size_t i{0}; i < size; ++i) {
        //     if (o[i] != bytes[i]) return false;
        // }

        uint16_t n_chunk = size >> 3;
        const uint64_t *p1 = reinterpret_cast<const uint64_t*>(&o[0]);
        const uint64_t *p2 = reinterpret_cast<const uint64_t*>(&bytes[0]);
        for (size_t i{0}; i < n_chunk; ++i) {
            if ((*p1) != (*p2)) return false;
            ++p1; ++p2;
        }
        return true;
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
namespace std {

template<size_t size>
struct hash<big_int<size>> {
    size_t operator()(const big_int<size>& o) const noexcept {
        return std::hash<string>{}(o.to_hex_string());
    }
};

}

template struct big_int<n_bytes>;
template struct big_int<16>;

using dhash_t = big_int<n_bytes>;
using md5_t = big_int<16>;
template<size_t size>
using sample_t = pair<string, big_int<size>>;


////////////////////////////// big_int

//////////////////////////////
/* 
 * topology class for comparing pairs
 */
//////////////////////////////

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

    friend ostream& operator<<(ostream& os, const triangle_t& tr) {
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

    friend ostream& operator<<(ostream& os, const pair_t& p) {
        os << "i=" << p.i << ", j=" << p.j << ", diff=" << p.diff;
        return os;
    }

    template<size_t size>
    static vector<string> inds_to_strings_vector(const vector<sample_t<size>>& samples, 
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


////////////////////////////// comparing topology



//////////////////////////////
/* 
 * declaritions of all functions
 */
//////////////////////////////

template<size_t size>
ostream& operator<<(ostream& os, const sample_t<size>& one);

string read_bin(string path);

vector<string> read_lines(string inpth);

template<typename T>
void save_result(vector<T>& res, string savepth);

dhash_t compute_dhash(string buf, uint16_t hw);

void gen_dhashes_worker(size_t tid, size_t n_proc,
        vector<string>& v_paths, vector<sample_t<n_bytes>>& res, uint16_t hw);

void gen_all_dhash(string inpth, size_t n_proc);

void filter_out_images(string inpth, size_t n_proc);

template<size_t size>
vector<sample_t<size>> parse_hashes(string inpth); 

template<size_t size>
vector<pair_t> down_triangle_worker(size_t tid, size_t n_proc, 
        vector<sample_t<size>>& samples);

template<size_t size>
vector<pair_t> get_dup_pairs_down_triangle(vector<sample_t<size>>& samples, size_t n_proc);

vector<size_t> remove_dups_from_pairs(const vector<pair_t>& dup_pairs); 

template<typename T>
vector<T> vector_remove_by_inds(vector<T>& samples, vector<size_t>& remove_inds);

template<size_t size>
vector<string> samples_to_hashes(vector<sample_t<size>>& samples);

template<size_t size>
vector<size_t> rectangle_worker(size_t tid, size_t n_proc, 
        vector<sample_t<size>>& samples1, 
        vector<sample_t<size>>& samples2);

template<size_t size>
vector<size_t> get_dup_inds_rectangle(vector<sample_t<size>>& samples1, 
        vector<sample_t<size>>& samples2, size_t n_proc);

void dedup_one_dataset_dhash(string inpth, size_t n_proc);

void merge_datasets_dhash(vector<string> inpths, string savepth, size_t n_proc);

void gen_binhash_worker(size_t tid, size_t n_proc, 
        vector<string>& paths, vector<string>& res);

void gen_all_binhash(string inpth, size_t n_proc);

md5_t compute_md5(string& s);

void gen_md5_worker(size_t tid, size_t n_proc, 
        vector<string>& paths, vector<sample_t<16>>& res);

void gen_all_md5(string inpth, size_t n_proc);

template<size_t size>
vector<sample_t<size>> remove_iden_by_hash(vector<sample_t<size>> samples);

template<size_t size>
void remove_iden_by_hash(string inpth);


//////////////////////////////
/* 
 * a wrapper of a method to compute md5
 */
//////////////////////////////




template<size_t size>
ostream& operator<<(ostream& os, const sample_t<size>& one) {
    os << one.first << "," << one.second;
    return os;
}


void print_string_as_hex(string& str) {
    cout << std::hex;
    for (char c : str) {
        cout << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
    }
}


string read_bin(string path) {
    // ifstream fin(path, ios::in);
    // stringstream ss;
    // fin >> ss.rdbuf();
    // fin.close();
    // return ss.str();

    ifstream fin(path, ios::in|ios::binary);
    if (!fin.is_open()) {
        cerr << "[ERROR] open for read fail: " << path 
            << ", errno: " << errno << endl;
    }

    fin.seekg(0, fin.end); fin.clear();
    size_t len = fin.tellg();
    fin.seekg(0); fin.clear();

    string res; res.resize(len);
    fin.read(&res[0], len);
    fin.close();
    return res;
}


vector<string> read_lines(string inpth) {
    ifstream fin(inpth, ios::in);
    if (!fin.is_open()) {
        cerr << "[ERROR] open for read fail: " << inpth 
            << ", errno: " << errno << endl;
    }
    stringstream ss;
    fin >> ss.rdbuf();
    fin.close();

    string buf;

    ss.clear();ss.seekg(0, ios::beg);
    vector<string> lines;
    while (getline(ss, buf)) {
        lines.push_back(buf);
    }
    return lines;
}

vector<string> filter_img_worker(size_t tid, size_t n_proc, vector<string>& v_paths) {

    /// rules
    // jpg/jpeg: starts with ff d8, ends with ff d9
    // png: starts with 89 50 4e 47 0d 0a 1a 0a, and ends with 49 45 4e 44 ae 42 60 82
    // file size less than 50k
    // shorter side less than 64
    // longer side greater than 2048
    // ratio of longer and shorter greater than 4
    // image channel number is not 3

    const string HEAD_JPG{'\xff', '\xd8'};
    const string TAIL_JPG{'\xff', '\xd9'};
    const string HEAD_PNG({'\x90', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a'});
    const string TAIL_PNG({'\x49', '\x45', '\x4e', '\x44', '\xae', '\x42', '\x60', '\x82'});
    const string POSTF_JPG1("jpg");
    const string POSTF_JPG2("jpeg");
    const string POSTF_PNG("png");

    auto lam_check_func = [&](const string pth)->bool {
        // postfix
        string postf = pth.substr(pth.rfind(".")+1);
        std::transform(postf.begin(), postf.end(), postf.begin(), [](char ch){
                return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                });

        // file size
        std::ifstream fin(pth, std::ifstream::in);
        fin.seekg(0, fin.end);
        size_t size = fin.tellg();
        if (size < 50000) return false;

        // head bytes and tail bytes
        fin.clear(); fin.seekg(0); fin.clear();
        string head, tail;
        size_t nbytes = 0;
        if (postf == POSTF_JPG1 or postf == POSTF_JPG2) {
            nbytes = 2;
        } else if (postf == POSTF_PNG) {
            nbytes = 8;
        }
        head.resize(nbytes); tail.resize(nbytes);
        fin.read(&head[0], nbytes);
        fin.seekg(size - nbytes);
        fin.read(&tail[0], nbytes);

        if (postf == POSTF_JPG1 or postf == POSTF_JPG2) {
            if (head != HEAD_JPG or tail != TAIL_JPG) return false;
        } else if (postf == POSTF_PNG) {
            if (head != HEAD_PNG or tail != TAIL_PNG) return false;
        }

        // image w/h/c/ratio
        cv::Mat im = cv::imread(pth);
        int64_t H = im.rows;
        int64_t W = im.cols;
        int64_t C = im.channels();
        float ratio = static_cast<float>(H) / static_cast<float>(W);
        if (W > H) {
            ratio = static_cast<float>(W) / static_cast<float>(H);
        }

        if (std::min(W, H) < 64) return false;
        if (std::max(W, H) > 2048) return false;
        if (C != 3) return false;
        if (ratio > 4.F) return false;

        fin.close();

        return true;
    };

    size_t n_samples = v_paths.size();
    vector<string> res; res.reserve(static_cast<size_t>(n_samples / n_proc + 1));
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        bool valid = lam_check_func(v_paths[i]);
        if (valid) {
            res.push_back(v_paths[i]);
        }
    }
    return res;
}


/// filter out images by some rule
void filter_out_images(string inpth, size_t n_proc) {
    cout << "filter out images by rules" << endl;
    vector<string> paths = read_lines(inpth);
    vector<string> res; res.reserve(paths.size());

    vector<std::future<vector<string>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, filter_img_worker, 
                                  tid, n_proc, std::ref(paths));
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }

    save_result(res, inpth + ".filt");
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
    if (!fout.is_open()) {
        cerr << "[ERROR] open for write fail: " << savepth 
            << ", errno: " << errno << endl;
    }
    ss >> fout.rdbuf(); fout.close();
}


dhash_t compute_dhash(string buf, uint16_t hw) {
    cv::Mat im = cv::imread(buf, cv::IMREAD_GRAYSCALE);
    if (im.empty()) cout << "empty image: " << buf << endl;

    cv::Mat resize;
    cv::resize(im, resize, cv::Size(hw + 1, hw + 1), 0, 0, cv::INTER_AREA);

    uint8_t *data = static_cast<uint8_t*>(resize.data);

    dhash_t hash;
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
                hash[byte_ind++] = byte;
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
                hash[byte_ind++] = byte;
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
        vector<string>& v_paths, vector<sample_t<n_bytes>>& res, uint16_t hw) {

    size_t n_samples = v_paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        // cout << "proc " << tid << ": " << v_paths[i] << endl;
        auto dhash = compute_dhash(v_paths[i], hw);
        res[i] = make_pair(v_paths[i], dhash);
    }
}


/// generate dhashes given paths of images
void gen_all_dhash(string inpth, size_t n_proc) {
    cout << "generate dhash" << endl;
    vector<string> paths = read_lines(inpth);
    vector<sample_t<n_bytes>> res; res.resize(paths.size());

    uint16_t hw = std::sqrt(n_bytes * 8 / 2);
    if (hw % 8 != 0) {
        cout << "[warning]: num of bytes for dhash should be divisible by 8" << endl;
    }

    vector<std::future<void>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, gen_dhashes_worker, 
                                  tid, n_proc, std::ref(paths), 
                                  std::ref(res), hw);
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid].wait();
    }

    save_result(res, inpth + ".dhash");
}


// read generated hash
template<size_t size>
vector<sample_t<size>> parse_hashes(string inpth) {

    auto lines = read_lines(inpth);
    size_t n_samples = lines.size();

    vector<sample_t<size>> res; res.reserve(n_samples);

    for (string& l : lines) {
        size_t pos = l.find(",");
        string key = l.substr(0, pos);
        string v = l.substr(pos + 1);

        res.push_back(std::make_pair(key, big_int<size>(v)));
    }
    return res;
}



template<size_t size>
vector<pair_t> down_triangle_worker(size_t tid, size_t n_proc, vector<sample_t<size>>& samples) {
    size_t n_samples = samples.size();
    // size_t res_size = n_samples * (n_samples + 1) / 2 / n_proc + 1;
    size_t res_size = 20000; // too large will cause bad allocate memory error
    vector<pair_t> res; res.reserve(res_size);

    // cout << "pre-allocated res size of tid " << tid << " is: " << res_size << endl;
    // cout << "tid " << tid << "res.max_size: " << res.max_size() << endl;

    triangle_t coo = triangle_t(tid);
    while (coo.is_in_range(n_samples - 1)) {
        size_t i = coo.get_i() + 1;
        size_t j = coo.get_j();
        uint16_t diff = big_int<size>::count_diff_bits(samples[i].second, samples[j].second);

        if (diff < thr) {
            res.push_back(pair_t(i, j, diff));
            if (res.size() % 500 == 0) {
                cout << "result size of tid " << tid << " is : " << res.size() << endl;
            }
        }

        coo.step(n_proc);
    }

    // cout << "tid " << tid << "work done\n";
    return res;
}


template<size_t size>
vector<pair_t> get_dup_pairs_down_triangle(vector<sample_t<size>>& samples, size_t n_proc) {

    vector<std::future<vector<pair_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async,
                down_triangle_worker<size>, tid, n_proc, std::ref(samples));
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


template<size_t size>
vector<string> samples_to_hashes(vector<sample_t<size>>& samples) {
    size_t num = samples.size();
    vector<string> res; res.reserve(num);

    for (auto& el : samples) {
        res.push_back(el.first);
    }
    return res;
}



template<size_t size>
vector<size_t> rectangle_worker(size_t tid, size_t n_proc, 
                                vector<sample_t<size>>& samples1, 
                                vector<sample_t<size>>& samples2) {
    vector<size_t> res;
    size_t n_loop = samples1.size();
    size_t n_gallery = samples2.size();
    for (size_t i{tid}; i < n_loop; i += n_proc) {
        for (size_t j{0}; j < n_gallery; ++j) {
            uint16_t diff = dhash_t::count_diff_bits(samples1[i].second, samples2[j].second);
            if (diff < thr) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}


template<size_t size>
vector<size_t> get_dup_inds_rectangle(vector<sample_t<size>>& samples1, 
                                       vector<sample_t<size>>& samples2, size_t n_proc) {

    vector<std::future<vector<size_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, rectangle_worker<size>, 
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




/// dedup within one dataset using dhash
void dedup_one_dataset_dhash(string inpth, size_t n_proc) {

    cout << "read file" << endl;
    auto samples = parse_hashes<n_bytes>(inpth);
    cout << "num of samples: " << samples.size() << endl;

    cout << "remove samples with indentical dhash code" << endl;
    size_t before = samples.size();
    samples = remove_iden_by_hash(samples);
    size_t after = samples.size();
    cout << "from " << before << " to " << after << endl;

    cout << "compare each pair" << endl;
    auto dup_pairs_ind = get_dup_pairs_down_triangle(samples, n_proc);
    auto dup_pairs_str = pair_t::inds_to_strings_vector(samples, dup_pairs_ind);
    save_result(dup_pairs_str, inpth + ".pair");

    cout << "search for to-be-removed" << endl;
    auto to_be_removed = remove_dups_from_pairs(dup_pairs_ind);

    cout << "remove inds and save" << endl;
    auto deduped = vector_remove_by_inds<sample_t<n_bytes>>(samples, to_be_removed);
    save_result(deduped, inpth + ".dedup.dhash");
    auto res = samples_to_hashes(deduped);
    cout << "num of remaining samples: " << res.size() << endl;

    save_result(res, inpth + ".dedup");

}


/// merge multi datasets together, each of which has already been deduplicated
void merge_datasets_dhash(vector<string> inpths, string savepth, size_t n_proc) {
    
    vector<sample_t<n_bytes>> deduped;
    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        cout << "read and merge " << inpths[i] << endl;
        auto samplesi = parse_hashes<n_bytes>(inpths[i]);

        if (deduped.size() > 0) {
            auto dup_inds_src = get_dup_inds_rectangle(samplesi, deduped, n_proc);
            samplesi = vector_remove_by_inds<sample_t<n_bytes>>(samplesi, dup_inds_src);
        }
        std::copy(samplesi.begin(), samplesi.end(), std::back_inserter(deduped));
    }

    auto res = samples_to_hashes(deduped);
    save_result(res, savepth);
}


/// drop those paths in src_path which duplicates with some path in dst_path
void remain_datasets_dhash(string src_path, string dst_path, size_t n_proc) {

    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;
    vector<sample_t<n_bytes>> remains;
    auto src_samples = parse_hashes<n_bytes>(src_path);
    auto dst_samples = parse_hashes<n_bytes>(dst_path);
    auto dup_inds_src = get_dup_inds_rectangle(src_samples, dst_samples, n_proc);
    src_samples = vector_remove_by_inds<sample_t<n_bytes>>(src_samples, dup_inds_src);

    auto res = samples_to_hashes(src_samples);
    string savepth = src_path + ".rem";
    save_result(res, savepth);
}



/// dedup within one dataset using cpp hash, not necessarily md5, can be other hash methods(not for sure)
void gen_binhash_worker(size_t tid, size_t n_proc, 
        vector<string>& paths, vector<string>& res) {
    size_t n_samples = paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        string bin = read_bin(paths[i]);
        size_t hv = std::hash<string>{}(bin);
        stringstream ss;
        ss << paths[i] << "," << hv;
        res[i] = ss.str();
    }
}

void gen_all_binhash(string inpth, size_t n_proc) {
    cout << "gen bin hash with std::hash_t \n";
    vector<string> paths = read_lines(inpth);
    vector<string> res; res.resize(paths.size());

    vector<std::future<void>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, gen_binhash_worker, 
                                  tid, n_proc, std::ref(paths), 
                                  std::ref(res));
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid].wait();
    }

    save_result(res, inpth + ".binhash");
}


md5_t compute_md5(string& s) {

    const EVP_MD *md = EVP_md5();
    unsigned int md_len = EVP_MD_size(md); // 16

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();;
    EVP_DigestInit_ex(mdctx, md, nullptr);

    EVP_DigestUpdate(mdctx, &s[0], s.size());

    big_int<16> md_value;
    EVP_DigestFinal_ex(mdctx, &md_value[0], &md_len);
    EVP_MD_CTX_free(mdctx);
    return md_value;
}


void gen_md5_worker(size_t tid, size_t n_proc, 
        vector<string>& paths, vector<sample_t<16>>& res) {
    size_t n_samples = paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        string bin = read_bin(paths[i]);
        md5_t one = compute_md5(bin);
        res[i] = make_pair(paths[i], one);
    }
}


void gen_all_md5(string inpth, size_t n_proc) {
    cout << "gen all md5 \n";
    vector<string> paths = read_lines(inpth);
    vector<sample_t<16>> res; res.resize(paths.size());

    vector<std::future<void>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, gen_md5_worker, 
                                  tid, n_proc, std::ref(paths), 
                                  std::ref(res));
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid].wait();
    }

    save_result(res, inpth + ".md5");
}


template<size_t size>
vector<sample_t<size>> remove_iden_by_hash(vector<sample_t<size>> samples) {
    unordered_map<big_int<size>, string> hash;
    for (auto& one : samples) {
        string path = one.first;
        big_int<size> code = one.second;
        hash[code] = path;
    }
    vector<sample_t<size>> res; res.reserve(hash.size());
    for (auto& one : hash) {
        big_int<size> code = one.first;
        string path = one.second;
        res.push_back(std::make_pair(path, code));
    }
    return res;
}


template<size_t size>
void remove_iden_by_hash(string inpth) {

    cout << "remove indentical by hash code \n";
    auto samples = parse_hashes<size>(inpth);
    cout << "num samples before dedup: " << samples.size() << endl;

    samples = remove_iden_by_hash(samples);
    auto res = samples_to_hashes(samples);

    cout << "num samples after dedup: " << res.size() << endl;
    save_result(res, inpth + ".dedup");
}



/* 
 *     === 
 *     === compile command:  
 *     === 
 *
 *         $ apt install libopencv-dev libssl-dev
 *         $ g++ -O2 main.cpp -std=c++14 -o main -lpthread -lcrypto $(pkg-config --libs --cflags opencv4)
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
    auto get_n_proc = [](const char* s) {
        size_t n_proc = static_cast<size_t>(std::stoi(string(s)));
        return n_proc;
    };

    if (cmd == "filter") {
        size_t n_proc = get_n_proc(argv[2]);
        string inpth(argv[3]);
        filter_out_images(inpth, n_proc);

    } else if (cmd == "gen_dhash") {

        size_t n_proc = get_n_proc(argv[2]);
        string inpth(argv[3]);
        gen_all_dhash(inpth, n_proc);

    } else if (cmd == "dedup_dhash") {

        size_t n_proc = get_n_proc(argv[2]);
        string inpth(argv[3]);
        dedup_one_dataset_dhash(inpth, n_proc);

    } else if (cmd == "merge_dhash") {

        size_t n_proc = get_n_proc(argv[2]);
        vector<string> inpths;
        for (int i{3}; i < argc - 1; ++i) {
            inpths.push_back(argv[i]);
        }
        string savepth(argv[argc - 1]);
        merge_datasets_dhash(inpths, savepth, n_proc);

    } else if (cmd == "remain_dhash") {

        size_t n_proc = get_n_proc(argv[2]);
        string src_path = argv[3];
        string dst_path = argv[4];
        remain_datasets_dhash(src_path, dst_path, n_proc);

    } else if (cmd == "gen_md5") {

        size_t n_proc = get_n_proc(argv[2]);
        string inpth(argv[3]);
        gen_all_md5(inpth, n_proc);

    } else if (cmd == "dedup_md5") {

        string inpth(argv[3]);
        remove_iden_by_hash<16>(inpth);
    }


    return 0;
}

