#ifndef _SAMPLES_H_
#define _SAMPLES_H_

#include <string>
#include <vector>
#include <functional>

#include "hash_func.h"


using namespace std;




struct sample_set {
public:

    sample_set();
    sample_set(const sample_set&);
    sample_set(sample_set&&);

    void set_n_proc(const int n_proc);

    void concat_other(const sample_set&);
    void remove_by_inds(const vector<size_t>&);

    void load_keys(const string&);
    void cleanup_keys(const bool keep_memory);
    void filter_by_keys(std::function<bool(const string&)>);
    void save_keys(const string&) const;

    void load_samples_dhash(const string&);
    void cleanup_dhash(const bool keep_memory);
    void gen_all_dhashes();
    void dedup_by_dhash();
    void merge_other_dhash(const sample_set&);
    void drop_exists_by_dhash(const sample_set&);
    void save_samples_dhash(const string&) const;

    void load_samples_md5(const string&);
    void cleanup_md5(const bool keep_memory);
    void gen_all_md5s();
    void dedup_by_md5();
    void save_samples_md5(const string&) const;

    // void gen_all_binhashes();
    // void save_samples_binhash(const string&) const;


private:
    vector<string> keys;
    vector<dhash_t> v_dhash;
    vector<md5_t> v_md5;
    // vector<binhash_t> v_binhash;

    size_t n_proc;


};

#endif

