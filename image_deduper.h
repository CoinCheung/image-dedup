#ifndef _IMAGE_DEDUPER_H_
#define _IMAGE_DEDUPER_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include "samples.h"


using namespace std;



struct ImageDeduper {
public:

    ImageDeduper(); 

    void parse_args(int argc, char* argv[]);

    void run_cmd();

    void filter_out_images();

    void gen_all_dhash();
    void dedup_one_dataset_dhash();
    void merge_datasets_dhash();
    void remain_datasets_dhash();

    void gen_all_md5();
    void dedup_one_dataset_md5();

    // void gen_all_binhash();
    // void dedup_one_dataset_binhash();

    void process_pipeline();

private:
    string m_cmd;
    vector<string> m_src_paths;
    vector<string> m_dst_paths;

    sample_set m_samples;

    unordered_map<string, function<void(void)>> m_funcs;
};


#endif
