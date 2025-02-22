
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <cmath>
#include <functional>

#include <opencv2/opencv.hpp>

#include "samples.hpp"
#include "topology.h"
#include "image_deduper.h"
#include "hash_func.h"
#include "helper.h"


using namespace std;


// uint16_t thr = 10;
// uint16_t thr = 30;
uint16_t thr = 200;



//////////////////////////////
/* 
 * declarations of dependant functions
 */
//////////////////////////////


namespace {
    
vector<size_t> remove_dups_from_pairs(const vector<pair_t>&);

vector<string> filter_img_worker(size_t tid, size_t n_proc, vector<string>& v_paths);

template<typename hash_t>
void gen_samples_worker(size_t tid, size_t n_proc,
        vector<string>& paths, sample_set<hash_t>& res,
        std::function<hash_t(const string&)> func);

template<typename hash_t>
void gen_all_samples(size_t n_proc, vector<string>& paths, 
        sample_set<hash_t>& res, std::function<hash_t(const string&)> hash_func);

template<typename hash_t>
vector<pair_t> down_triangle_worker(size_t, size_t, sample_set<hash_t>&);

template<typename hash_t>
vector<pair_t> get_dup_pairs_down_triangle(sample_set<hash_t>&, size_t);

template<typename hash_t>
vector<size_t> rectangle_worker(size_t, size_t, sample_set<hash_t>&, sample_set<hash_t>&);

template<typename hash_t>
vector<size_t> get_dup_inds_rectangle(sample_set<hash_t>&, sample_set<hash_t>&, size_t);

} // namespace


//////////////////////////////
/* 
 * implement of member functions
 */
//////////////////////////////

ImageDeduper::ImageDeduper() {}


void ImageDeduper::parse_args(int argc, char* argv[]) {
    cmd = argv[1];
    auto get_n_proc = [](const char* s) {
        size_t n_proc = static_cast<size_t>(std::stoi(string(s)));
        return n_proc;
    };

    if (cmd == "filter") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);

    } else if (cmd == "gen_md5") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);

    } else if (cmd == "dedup_md5") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);

    } else if (cmd == "gen_dhash") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);

    } else if (cmd == "dedup_dhash") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);

    } else if (cmd == "merge_dhash") {
        n_proc = get_n_proc(argv[2]);
        for (int i{3}; i < argc - 1; ++i) {
            src_paths.push_back(argv[i]);
        }
        dst_paths.push_back(argv[argc - 1]);

    } else if (cmd == "remain_dhash") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);
        dst_paths.push_back(argv[4]);
    }
}


void ImageDeduper::run_cmd() {
    if (cmd == "filter") {
        filter_out_images();
    } else if (cmd == "gen_md5") {
        gen_all_md5();
    } else if (cmd == "dedup_md5") {
        dedup_one_dataset_md5();
    } else if (cmd == "gen_dhash") {
        gen_all_dhash();
    } else if (cmd == "dedup_dhash") {
        dedup_one_dataset_dhash();
    } else if (cmd == "merge_dhash") {
        merge_datasets_dhash();
    } else if (cmd == "remain_dhash") {
        remain_datasets_dhash();
    }
}


void ImageDeduper::filter_out_images() {
    /// filter out images by some rule
    string inpth = src_paths[0];
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


void ImageDeduper::gen_all_dhash() {
    /// generate dhashes given paths of images
    string inpth = src_paths[0];
    cout << "generate dhash" << endl;
    vector<string> paths = read_lines(inpth);

    sample_set<dhash_t> res; res.resize(paths.size());

    uint16_t hw = std::sqrt(n_bytes * 8 / 2);
    if (hw % 8 != 0) {
        cout << "[warning]: num of bytes for dhash should be divisible by 8" << endl;
    }

    // must use std::function as ret type here
    // std::async cannot recognize if we use auto
    using func_t = std::function<dhash_t(const string&)>; 
    func_t dhash_func = std::bind(compute_dhash, std::placeholders::_1, hw);
    gen_all_samples<dhash_t>(n_proc, paths, res, dhash_func);

    res.save_samples(inpth + ".dhash");
}


void ImageDeduper::dedup_one_dataset_dhash() {
    /// dedup within one dataset using dhash
    cout << "remove samples with indentical dhash code" << endl;

    string inpth = src_paths[0];
    sample_set<dhash_t> samples(inpth);

    size_t before = samples.size();
    samples.dedup_by_identical_hash();
    size_t after = samples.size();
    cout << "from " << before << " to " << after << endl;

    cout << "compare each pair" << endl;
    auto dup_pairs_ind = get_dup_pairs_down_triangle(samples, n_proc);
    auto dup_pairs_str = pair_t::inds_to_strings_vector<dhash_t>(samples, dup_pairs_ind);
    save_result(dup_pairs_str, inpth + ".pair");

    cout << "search for to-be-removed" << endl;
    auto to_be_removed = remove_dups_from_pairs(dup_pairs_ind);

    cout << "remove inds and save" << endl;
    samples.remove_by_given_inds(to_be_removed);
    cout << "num of remaining samples: " << samples.size() << endl;
    samples.save_samples(inpth + ".dedup.dhash");
    samples.save_keys(inpth + ".dedup");
}


void ImageDeduper::merge_datasets_dhash() {
    /// merge multi datasets together, each of which has already been deduplicated

    vector<string> inpths = src_paths;
    string savepth = dst_paths[0];

    sample_set<dhash_t> deduped;
    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        cout << "read and merge " << inpths[i] << endl;
        sample_set<dhash_t> samplesi(inpths[i]);

        if (deduped.size() > 0) {
            auto dup_inds_src = get_dup_inds_rectangle(samplesi, deduped, n_proc);
            samplesi.remove_by_given_inds(dup_inds_src);
        }

        deduped.merge_other(samplesi);
    }
    deduped.save_keys(savepth);
}


void ImageDeduper::remain_datasets_dhash() {

    string src_path = src_paths[0];
    string dst_path = dst_paths[0];

    /// drop those paths in src_path which duplicates with some path in dst_path
    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;
    sample_set<dhash_t> src_samples(src_path);
    sample_set<dhash_t> dst_samples(dst_path);
    auto dup_inds_src = get_dup_inds_rectangle(src_samples, dst_samples, n_proc);
    src_samples.remove_by_given_inds(dup_inds_src);
    src_samples.save_keys(src_path + ".rem");
}


void ImageDeduper::gen_all_md5() {

    string inpth = src_paths[0];

    cout << "gen all md5 \n";
    vector<string> paths = read_lines(inpth);
    sample_set<md5_t> res; res.resize(paths.size());

    using func_t = std::function<md5_t(const string&)>; 
    using func_ptr = md5_t(*)(const string&);
    func_t md5_func = static_cast<func_ptr>(&compute_md5);
    gen_all_samples<md5_t>(n_proc, paths, res, md5_func);

    res.save_samples(inpth + ".md5");
}


void ImageDeduper::dedup_one_dataset_md5() {
    cout << "remove indentical by hash code \n";

    string inpth = src_paths[0];
    sample_set<md5_t> samples(inpth);

    cout << "num samples before dedup: " << samples.size() << endl;
    samples.dedup_by_identical_hash();
    cout << "num samples after dedup: " << samples.size() << endl;

    samples.save_keys(inpth + ".dedup");
}


void ImageDeduper::gen_all_binhash() {
    string inpth = src_paths[0];

    cout << "gen bin hash with std::hash_t \n";
    vector<string> paths = read_lines(inpth);
    sample_set<binhash_t> res; res.resize(paths.size());

    using func_t = std::function<binhash_t(const string&)>; 
    using func_ptr = binhash_t(*)(const string&);
    func_t binhash_func = static_cast<func_ptr>(&compute_binbash);

    gen_all_samples<binhash_t>(n_proc, paths, res, binhash_func);

    res.save_samples(inpth + ".binhash");
}




//////////////////////////////
/* 
 * implementation of dependant functions
 */
//////////////////////////////

namespace {

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


template<typename hash_t>
void gen_samples_worker(size_t tid, size_t n_proc,
        vector<string>& paths, sample_set<hash_t>& res,
        std::function<hash_t(const string&)> func) {

    size_t n_samples = paths.size();
    for (size_t i{tid}; i < n_samples; i += n_proc) {
        auto code = func(paths[i]);
        res[i].set(paths[i], code);
    }
}

template<typename hash_t>
void gen_all_samples(size_t n_proc, vector<string>& paths, 
        sample_set<hash_t>& res, std::function<hash_t(const string&)> hash_func) {

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
vector<pair_t> down_triangle_worker(size_t tid, size_t n_proc, sample_set<hash_t>& samples) {
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
        uint16_t diff = hash_t::count_diff_bits(samples[i].hash, samples[j].hash);

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


template<typename hash_t>
vector<pair_t> get_dup_pairs_down_triangle(sample_set<hash_t>& samples, size_t n_proc) {

    vector<std::future<vector<pair_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async,
                down_triangle_worker<hash_t>, tid, n_proc, std::ref(samples));
    }

    vector<pair_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}



template<typename hash_t>
vector<size_t> rectangle_worker(size_t tid, size_t n_proc, 
                                sample_set<hash_t>& samples1, 
                                sample_set<hash_t>& samples2) {
    vector<size_t> res;
    size_t n_loop = samples1.size();
    size_t n_gallery = samples2.size();
    for (size_t i{tid}; i < n_loop; i += n_proc) {
        for (size_t j{0}; j < n_gallery; ++j) {
            uint16_t diff = dhash_t::count_diff_bits(samples1[i].hash, samples2[j].hash);
            if (diff < thr) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}


template<typename hash_t>
vector<size_t> get_dup_inds_rectangle(sample_set<hash_t>& samples1, 
                                       sample_set<hash_t>& samples2, size_t n_proc) {

    vector<std::future<vector<size_t>>> futures(n_proc);
    for (size_t tid = 0; tid < n_proc; ++tid) {
        futures[tid] = std::async(std::launch::async, rectangle_worker<hash_t>, 
                                  tid, n_proc, std::ref(samples1), std::ref(samples2));
    }

    vector<size_t> res;
    for (size_t tid = 0; tid < n_proc; ++tid) {
    
        auto r = futures[tid].get();
        std::copy(r.begin(), r.end(), std::back_inserter(res));
    }
    return res;
}

}
