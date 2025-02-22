#ifndef _IMAGE_DEDUPER_H_
#define _IMAGE_DEDUPER_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>


using namespace std;



struct ImageDeduper {
public:
    string cmd;
    size_t n_proc;
    vector<string> src_paths;
    vector<string> dst_paths;

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

    void gen_all_binhash();
};


#endif
