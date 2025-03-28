
#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <iterator>
#include <future>
#include <cmath>
#include <functional>

#include "samples.h"
#include "hash_func.h"
#include "topology.h"
#include "utils.h"

// std::ios_base::sync_with_stdio(false);
using namespace std;




//////////////////////////////
/* 
 * declarations of dependant functions
 */
//////////////////////////////
namespace {



template<typename T>
void cleanup_vector(vector<T>&, const bool);

template<typename T>
void load_keys_values(const string&, vector<string>&, vector<T>&);

template<typename... Ts>
void save_samples(const string& savepth, const vector<string>& keys, 
        const vector<Ts>&... values);

vector<string> filter_img_worker(size_t tid, size_t n_proc, 
        const vector<string>&, std::function<bool(const string&)>);
auto img_keep_func(const string&)->bool;

template<typename hash_t>
void gen_samples_worker(size_t tid, size_t n_proc,
        vector<string>& paths, vector<hash_t>& res,
        std::function<hash_t(const string&)> func);
template<typename hash_t>
void gen_all_samples(size_t n_proc, vector<string>& paths, 
        vector<hash_t>& res, std::function<hash_t(const string&)> hash_func);

template<typename hash_t>
void dedup_by_identical_hash(vector<string>&, vector<hash_t>&);

template<typename hash_t>
vector<size_t> dedup_by_hash_bits_diff(vector<string>& keys, vector<hash_t>& v_hash, 
        size_t n_proc, const string& name, const uint16_t thr);

template<typename... Ts>
void remove_by_given_inds(const vector<size_t>& inds, vector<Ts>&... vecs);

vector<size_t> remove_dups_from_pairs(const vector<pair_t>&);

template<typename hash_t>
vector<pair_t> get_dup_pairs_down_triangle(const vector<hash_t>&, const size_t, const uint16_t);
template<typename hash_t>
vector<pair_t> down_triangle_worker(const size_t, const size_t, const vector<hash_t>&, const vector<uint16_t>&, const uint16_t);

template<typename hash_t>
vector<size_t> get_dup_inds_rectangle(const vector<hash_t>&, 
        const vector<hash_t>&, const size_t, const uint16_t);
template<typename hash_t>
vector<size_t> rectangle_worker(const size_t, const size_t, 
        const vector<hash_t>&, const vector<uint16_t>&, const vector<hash_t>&, const vector<uint16_t>&, const uint16_t);


} // namespace

//////////////////////////////
/* 
 * implement of member functions
 */
//////////////////////////////


sample_set::sample_set(): n_proc(4) {}


sample_set::sample_set(const sample_set& other) {
    n_proc  = other.n_proc;
    keys    = other.keys;
    v_md5   = other.v_md5;
    v_dhash = other.v_dhash;
    v_phash = other.v_phash;
}


sample_set::sample_set(sample_set&& other) {
    n_proc  = other.n_proc;
    std::swap(keys, other.keys);
    std::swap(v_md5, other.v_md5);
    std::swap(v_dhash, other.v_dhash);
    std::swap(v_phash, other.v_phash);
}


void sample_set::set_n_proc(const int n_proc) {
    this->n_proc = n_proc;
}


void sample_set::concat_other(const sample_set& other) {

    std::copy(other.keys.begin(),    other.keys.end(),    std::back_inserter(keys));
    std::copy(other.v_md5.begin(),   other.v_md5.end(),   std::back_inserter(v_md5));
    std::copy(other.v_dhash.begin(), other.v_dhash.end(), std::back_inserter(v_dhash));
    std::copy(other.v_phash.begin(), other.v_phash.end(), std::back_inserter(v_phash));
}


void sample_set::remove_by_inds(const vector<size_t>& inds) {
    unordered_set<size_t> us(inds.begin(), inds.end());
    size_t n_samples = keys.size();
    size_t pos = 0;
    for (size_t i{0}; i < n_samples; ++i) {
        if (us.find(i) != us.end()) continue;
        if (keys.size() > 0) keys[pos] = keys[i];
        if (v_dhash.size() > 0) v_dhash[pos] = v_dhash[i];
        if (v_phash.size() > 0) v_phash[pos] = v_phash[i];
        if (v_md5.size() > 0) v_md5[pos] = v_md5[i];
        ++pos;
    }
    if (keys.size() > 0) keys.resize(pos);
    if (v_dhash.size() > 0) v_dhash.resize(pos);
    if (v_phash.size() > 0) v_phash.resize(pos);
    if (v_md5.size() > 0) v_md5.resize(pos);
}

//// until here: add new fields

void sample_set::load_keys(const string& inpth) {
    cout << "\t- load from: " << inpth << endl;
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
    while (getline(ss, buf)) {
        keys.push_back(buf);
    }
}


void sample_set::cleanup_keys(const bool keep_memory) {
    cleanup_vector(keys, keep_memory);
}


void sample_set::filter_by_keys(std::function<bool(const string&)> func) {
    /// filter out images by some rule

    size_t num_before = keys.size();

    using worker_t = std::function<vector<string>(size_t)>;
    worker_t worker_func = std::bind(filter_img_worker, 
            std::placeholders::_1, n_proc, std::ref(keys), func);

    vector<std::future<vector<string>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, worker_func, tid); 
    }

    vector<string> res; res.reserve(keys.size());
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    std::swap(keys, res);

    cout << "\t- keys reduce from " << num_before << " to " << keys.size() << endl;
}


void sample_set::save_keys(const string& savepth) const {
    save_samples(savepth, keys);
    // stringstream ss;
    // std::copy(keys.begin(), keys.end(), ostream_iterator<string>(ss, "\n"));
    // ss.clear(); ss.seekg(0, ios::beg); 
    // ofstream fout(savepth, ios::out);
    // ss >> fout.rdbuf(); fout.close();
}


void sample_set::load_samples_dhash(const string& inpth) {
    load_keys_values(inpth, keys, v_dhash);
}


void sample_set::cleanup_dhash(const bool keep_memory) {
    cleanup_vector(v_dhash, keep_memory);
}


void sample_set::gen_all_dhashes() {
    uint16_t hw = std::sqrt(nbytes_dhash * 8 / 2);
    if (hw % 8 != 0) {
        cout << "[warning]: num of bytes for dhash should be divisible by 8" << endl;
    }

    // must use std::function as ret type here
    // std::async cannot recognize if we use auto
    using func_t = std::function<dhash_t(const string&)>; 
    func_t dhash_func = std::bind(compute_dhash, std::placeholders::_1, hw);
    gen_all_samples<dhash_t>(n_proc, keys, v_dhash, dhash_func);
}


void sample_set::dedup_by_dhash() {
    this->dedup_by_dhash("");
}


void sample_set::dedup_by_dhash(const string& savename) {
    dedup_by_identical_hash(keys, v_dhash);
    auto dup_inds = dedup_by_hash_bits_diff(keys, v_dhash, n_proc, savename, thr_dhash);

    cout << "\t- remove inds and save" << endl;
    this->remove_by_inds(dup_inds);
    cout << "\t- num of remaining samples: " << keys.size() << endl;
}


void sample_set::merge_other_dhash(const sample_set& other) {
    cout << "\t- merge" << endl;
    auto dup_inds_src = get_dup_inds_rectangle(other.v_dhash, v_dhash, n_proc, thr_dhash);
    sample_set src(other);
    src.remove_by_inds(dup_inds_src);
    this->concat_other(src);

}


void sample_set::drop_exists_by_dhash(const sample_set& other) {
    cout << "\t- find existing and drop" << endl;
    size_t num_before = keys.size();
    auto dup_inds = get_dup_inds_rectangle(v_dhash, other.v_dhash, n_proc, thr_dhash);
    this->remove_by_inds(dup_inds);
    cout << "\t- num of samples from " << num_before << " to " << keys.size() << endl;
}


void sample_set::save_samples_dhash(const string& savepth) const {
    save_samples<dhash_t>(savepth, keys, v_dhash);
}

void sample_set::load_samples_phash(const string& inpth) {
    load_keys_values(inpth, keys, v_phash);
}

void sample_set::cleanup_phash(const bool keep_memory) {
    cleanup_vector(v_phash, keep_memory);
}

void sample_set::gen_all_phashes() {
    size_t hw = static_cast<size_t>(std::sqrt(nbytes_phash << 3)) << 2;
    CHECK ((hw & 1) == 0) << "[error]: should resize image to even size, "
        << "nbytes_phash is: " << nbytes_phash 
        << ", resize size is: " << hw << endl;

    // must use std::function as ret type here
    // std::async cannot recognize if we use auto
    using func_t = std::function<phash_t(const string&)>; 
    func_t phash_func = std::bind(compute_phash, std::placeholders::_1, hw);
    gen_all_samples<phash_t>(n_proc, keys, v_phash, phash_func);
}

void sample_set::dedup_by_phash() {
    this->dedup_by_phash("");
}

void sample_set::dedup_by_phash(const string& savename) {
    dedup_by_identical_hash(keys, v_phash);
    auto dup_inds = dedup_by_hash_bits_diff(keys, v_phash, n_proc, savename, thr_phash);

    cout << "\t- remove dup samples" << endl;
    size_t num_before = keys.size();
    this->remove_by_inds(dup_inds);
    cout << "\t- num of samples from " << num_before << " to " << keys.size() << endl;
}

void sample_set::merge_other_phash(const sample_set& other) {
    cout << "\t- merge" << endl;
    auto dup_inds_src = get_dup_inds_rectangle(other.v_phash, v_phash, n_proc, thr_phash);
    sample_set src(other);
    src.remove_by_inds(dup_inds_src);
    this->concat_other(src);
}

void sample_set::drop_exists_by_phash(const sample_set& other) {
    cout << "\t- find existing and drop" << endl;
    size_t num_before = keys.size();
    auto dup_inds = get_dup_inds_rectangle(v_phash, other.v_phash, n_proc, thr_phash);
    this->remove_by_inds(dup_inds);
    cout << "\t- num of samples from " << num_before << " to " << keys.size() << endl;
}

void sample_set::save_samples_phash(const string& savepth) const {
    save_samples<phash_t>(savepth, keys, v_phash);
}


void sample_set::load_samples_md5(const string& inpth) {
    load_keys_values(inpth, keys, v_md5);
}


void sample_set::cleanup_md5(const bool keep_memory) {
    cleanup_vector(v_md5, keep_memory);
}


void sample_set::gen_all_md5s() {
    using func_t = std::function<md5_t(const string&)>; 
    using func_ptr = md5_t(*)(const string&);
    // func_t md5_func = static_cast<func_ptr>(&compute_md5);
    func_t md5_func = [](const string& s) {return compute_md5(s);};
    gen_all_samples<md5_t>(n_proc, keys, v_md5, md5_func);
}


void sample_set::dedup_by_md5() {
    dedup_by_identical_hash<md5_t>(keys, v_md5);
}


void sample_set::save_samples_md5(const string& savepth) const {
    save_samples(savepth, keys, v_md5);
}


// void sample_set::gen_all_binhashes() {
// }


// void sample_set::save_samples_binhash(const string& savepth) const {
// }


//////////////////////////////
/* 
 * implementation of dependant functions
 */
//////////////////////////////

namespace {


template<typename T>
void cleanup_vector(vector<T>& vec, const bool keep_memory) {
    if (keep_memory) {
        vec.resize(0);
    } else {
        vector<T>().swap(vec);
    }
}


template<typename hash_t>
void load_keys_values(const string& inpth, vector<string>& keys, vector<hash_t>& v_hash) {
    cout << "\t- load from: " << inpth << endl;
    CHECK (keys.size() == 0 and v_hash.size() == 0) << "keys or v_hash is not empty !!" << endl;

    ifstream fin(inpth, ios::in);
    CHECK(fin.is_open())  << "[ERROR] open for read fail: " << inpth << ", errno: " << errno << endl;

    stringstream ss; 
    fin >> ss.rdbuf(); fin.close();

    string buf;

    ss.clear();ss.seekg(0, ios::beg);
    while (getline(ss, buf)) {
        size_t pos = buf.find(",");
        string key = buf.substr(0, pos);
        string code = buf.substr(pos + 1);
        keys.push_back(key);
        v_hash.push_back(hash_t::create_from_string_hex(code));
    }
}


template<typename... Ts>
void save_samples(const string& savepth, const vector<string>& keys, 
        const vector<Ts>&... values) {

    CHECK (!((keys.size() != values.size()) || ...)) << "size of keys and values should be same !!" << endl;

    stringstream ss;
    size_t n_samples = keys.size();
    for (size_t i{0}; i < n_samples - 1; ++i) {
        ss << keys[i]; 
        ((ss << "," << values[i]), ...);
        ss << endl;
    }
    ss << keys[n_samples - 1]; 
    ((ss << "," << values[n_samples - 1]), ...);

    cout << "\t- save to " << savepth << endl;;
    ss.clear(); ss.seekg(0, ios::beg); 
    ofstream fout(savepth, ios::out);
    CHECK (fout.is_open())  << "[ERROR] open for write fail: " << savepth << ", errno: " << errno << endl;
    ss >> fout.rdbuf(); fout.close();
}


vector<string> filter_img_worker(size_t tid, size_t n_proc, 
        const vector<string>& keys, std::function<bool(const string&)> func) {

    size_t n_samples = keys.size();
    vector<string> res; res.reserve(static_cast<size_t>(n_samples / n_proc + 1));
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        bool valid = func(keys[i]);
        if (valid) {
            res.push_back(keys[i]);
        }
    }
    return res;
}


template<typename hash_t>
void gen_samples_worker(size_t tid, size_t n_proc,
        vector<string>& paths, vector<hash_t>& res,
        std::function<hash_t(const string&)> func) {

    size_t n_samples = paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        res[i] = func(paths[i]);
    }
}


template<typename hash_t>
void gen_all_samples(size_t n_proc, vector<string>& paths, 
        vector<hash_t>& res, std::function<hash_t(const string&)> hash_func) {

    res.resize(paths.size());

    auto gen_worker = std::bind(gen_samples_worker<hash_t>, 
            std::placeholders::_1, n_proc, std::ref(paths), 
            std::ref(res), hash_func);

    vector<std::future<void>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, gen_worker, tid);
    }

    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid].wait();
    }
}


template<typename hash_t>
void dedup_by_identical_hash(vector<string>& keys, vector<hash_t>& samples) {
    CHECK(keys.size() == samples.size()) << "size of keys and samples should be same !!" << endl;

    cout << "\t- remove samples with identical hash code" << endl;
    size_t n_samples = keys.size();
    // unordered_set<hash_t> hashset; // hash may collapse, so we use set
    set<hash_t> hashset;
    vector<size_t> inds; inds.reserve(static_cast<size_t>(n_samples * 0.2));
    for (size_t i{0}; i < n_samples; ++i) {
        if (hashset.find(samples[i]) != hashset.end()) {
            inds.push_back(i);
        }
        hashset.insert(samples[i]);
    }
    remove_by_given_inds(inds, keys, samples);
    cout << "\t- num of samples from " << n_samples << " to " << keys.size() << endl;
}


// TODO: here return inds, then see if we can remove inds for all hashes
template<typename hash_t>
vector<size_t> dedup_by_hash_bits_diff(vector<string>& keys, 
        vector<hash_t>& v_hash, size_t n_proc, const string& name, const uint16_t thr) {

    cout << "\t- compare each pair" << endl;
    auto dup_pairs_ind = get_dup_pairs_down_triangle(v_hash, n_proc, thr);

    if (name != "") {
        auto dup_pairs_str = pair_t::inds_to_strings_vector(keys, dup_pairs_ind);
        save_samples(name + ".pair", dup_pairs_str);
    }

    cout << "\t- search for to-be-removed" << endl;
    auto to_be_removed = remove_dups_from_pairs(dup_pairs_ind);
    return to_be_removed;

}



template<typename... Ts>
void remove_by_given_inds(const vector<size_t>& inds, vector<Ts>&... vecs) {

    unordered_set<size_t> to_removed_inds(inds.begin(), inds.end());
    size_t n_samples = (vecs.size(), ...);

    size_t pos = 0;
    for (size_t i{0}; i < n_samples; ++i) {
        if (to_removed_inds.find(i) == to_removed_inds.end()) {
            ((vecs[pos] = vecs[i]), ...);
            ++pos;
        }
    }
    (vecs.resize(pos), ...);
}


vector<size_t> remove_dups_from_pairs(const vector<pair_t>& dup_pairs) {
    unordered_map<size_t, unordered_set<size_t>> adj_table;
    for (auto& el : dup_pairs) {
        if (adj_table.find(el.m_i) == adj_table.end()) {
            adj_table[el.m_i] = unordered_set<size_t>();
        }
        if (adj_table.find(el.m_j) == adj_table.end()) {
            adj_table[el.m_j] = unordered_set<size_t>();
        }
        adj_table[el.m_i].insert(el.m_j);
        adj_table[el.m_j].insert(el.m_i);
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


template<typename hash_t>
vector<pair_t> get_dup_pairs_down_triangle(const vector<hash_t>& samples, 
        const size_t n_proc, const uint16_t thr) {

    vector<uint16_t> v_nbits_set; v_nbits_set.resize(samples.size());
    std::transform(samples.begin(), samples.end(), v_nbits_set.begin(), [](const hash_t& hash) {return hash.count_nbits_set();});

    vector<std::future<vector<pair_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async,
                down_triangle_worker<hash_t>, tid, n_proc, std::ref(samples), std::ref(v_nbits_set), thr);
    }

    vector<pair_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}


template<typename hash_t>
vector<pair_t> down_triangle_worker(const size_t tid, const size_t n_proc, 
        const vector<hash_t>& samples, const vector<uint16_t>& v_nbits_set, const uint16_t thr) {
    size_t n_samples = samples.size();
    // size_t res_size = n_samples * (n_samples + 1) / 2 / n_proc + 1;
    size_t res_size = 20000; // too large will cause bad allocate memory error
    vector<pair_t> res; res.reserve(res_size);

    triangle_t coo = triangle_t(tid);
    while (coo.is_in_range(n_samples - 1)) {
        size_t i = coo.get_i() + 1;
        size_t j = coo.get_j();

        uint16_t min_diff = std::max(v_nbits_set[i], v_nbits_set[j]) - std::min(v_nbits_set[i], v_nbits_set[j]);
        if (min_diff < thr) {
            uint16_t diff = hash_t::count_diff_bits(samples[i], samples[j]);

            if (diff < thr) {
                res.push_back(pair_t(i, j, diff));
                if (res.size() % 500 == 0) {
                    cout << "\t- result size of tid " << tid << " is : " << res.size() << endl;
                }
            }
        }

        coo.step(n_proc);
    }

    // cout << "tid " << tid << "work done\n";
    return res;
}


template<typename hash_t>
vector<size_t> get_dup_inds_rectangle(const vector<hash_t>& current, 
        const vector<hash_t>& other, const size_t n_proc, const uint16_t thr) {

    vector<uint16_t> v_nbits_set_cur; v_nbits_set_cur.resize(current.size());
    vector<uint16_t> v_nbits_set_oth; v_nbits_set_oth.resize(other.size());
    std::transform(current.begin(), current.end(), v_nbits_set_cur.begin(), [](const hash_t& hash) {return hash.count_nbits_set();});
    std::transform(other.begin(), other.end(), v_nbits_set_oth.begin(), [](const hash_t& hash) {return hash.count_nbits_set();});
    
    using func_t = std::function<vector<size_t>(const size_t&)>; 
    func_t worker_func = std::bind(rectangle_worker<hash_t>, 
            std::placeholders::_1, n_proc, std::ref(current), std::ref(v_nbits_set_cur),
            std::ref(other), std::ref(v_nbits_set_oth), thr);

    vector<std::future<vector<size_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, worker_func, tid); 
    }

    vector<size_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}


template<typename hash_t>
vector<size_t> rectangle_worker(const size_t tid, const size_t n_proc, 
        const vector<hash_t>& current, const vector<uint16_t>& v_nbits_set_cur, 
        const vector<hash_t>& other, const vector<uint16_t>& v_nbits_set_oth, const uint16_t thr) {

    vector<size_t> res;
    size_t n_loop = current.size();
    size_t n_gallery = other.size();
    for (size_t i{tid}; i < n_loop; i += n_proc) {
        for (size_t j{0}; j < n_gallery; ++j) {

            uint16_t min_diff = std::max(v_nbits_set_cur[i], v_nbits_set_oth[j]) - std::min(v_nbits_set_cur[i], v_nbits_set_oth[j]);
            if (min_diff >= thr) continue;

            uint16_t diff = hash_t::count_diff_bits(current[i], other[j]);
            if (diff < thr) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}


} // namespace
