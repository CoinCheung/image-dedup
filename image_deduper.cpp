
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "samples.h"
#include "image_deduper.h"
#include "image_filter.h"
#include "utils.h"


using namespace std;



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
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::filter_out_images, this);

    } else if (cmd == "gen_md5") {
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::gen_all_md5, this);

    } else if (cmd == "dedup_md5") {
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::dedup_one_dataset_md5, this);

    } else if (cmd == "gen_dhash") {
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::gen_all_dhash, this);

    } else if (cmd == "dedup_dhash") {
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::dedup_one_dataset_dhash, this);

    } else if (cmd == "merge_dhash") {
        for (int i{3}; i < argc - 1; ++i) {
            src_paths.push_back(argv[i]);
        }
        dst_paths.push_back(argv[argc - 1]);
        funcs[cmd] = std::bind(&ImageDeduper::merge_datasets_dhash, this);

    } else if (cmd == "remain_dhash") {
        src_paths.push_back(argv[3]);
        dst_paths.push_back(argv[4]);
        funcs[cmd] = std::bind(&ImageDeduper::remain_datasets_dhash, this);

    } else if (cmd == "pipeline") {
        src_paths.push_back(argv[3]);
        funcs[cmd] = std::bind(&ImageDeduper::process_pipeline, this);
    }

    size_t n_proc = get_n_proc(argv[2]);
    samples.set_n_proc(n_proc);
}


void ImageDeduper::run_cmd() {
     
    if (cmd == "pipeline") {
        process_pipeline();
    } else {
        Timer::run_func(funcs[cmd]);
    }
}


void ImageDeduper::filter_out_images() {
    /// filter out images by some rule
    cout << "filter out images by rules" << endl;

    string inpth = src_paths[0];

    samples.load_keys(inpth);
    samples.filter_by_keys(img_keep_func);
    samples.save_keys(inpth + ".filt");
}


void ImageDeduper::gen_all_dhash() {
    /// generate dhashes given paths of images
    cout << "generate all dhash" << endl;

    string inpth = src_paths[0];

    samples.load_keys(inpth);
    samples.gen_all_dhashes();
    samples.save_samples_dhash(inpth + ".dhash");
}


void ImageDeduper::dedup_one_dataset_dhash() {
    /// dedup within one dataset using dhash
    cout << "dedup by dhash" << endl;

    string inpth = src_paths[0];

    samples.load_samples_dhash(inpth); // TODO: here if samples is not empty, need to check whether inpth missing keys
    samples.dedup_by_dhash(inpth);
    samples.save_samples_dhash(inpth + ".dedup.dhash");
    samples.save_keys(inpth + ".dedup");
}


void ImageDeduper::merge_datasets_dhash() {
    /// merge multi datasets together, each of which has already been deduplicated
    cout << "merge multiple deduped datasets by dhash" << endl;

    vector<string> inpths = src_paths;
    string savepth = dst_paths[0];

    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        sample_set samplesi;
        samplesi.load_samples_dhash(inpths[i]);
        samples.merge_other_dhash(samplesi);
    }
    samples.save_keys(savepth);
}


void ImageDeduper::remain_datasets_dhash() {
    /// drop those paths in src_path which duplicates with some path in dst_path
    
    string src_path = src_paths[0];
    string dst_path = dst_paths[0];

    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;

    samples.load_samples_dhash(src_path);
    sample_set dst_samples;
    dst_samples.load_samples_dhash(dst_path);

    samples.drop_exists_by_dhash(dst_samples);
    samples.save_keys(src_path + ".rem");
}


void ImageDeduper::gen_all_md5() {

    cout << "gen all md5" << endl;

    string inpth = src_paths[0];

    samples.load_keys(inpth);
    samples.gen_all_md5s();
    samples.save_samples_md5(inpth + ".md5");
}


void ImageDeduper::dedup_one_dataset_md5() {
    cout << "remove indentical by md5 code" << endl;

    string inpth = src_paths[0];

    samples.load_samples_md5(inpth);
    samples.dedup_by_md5();
    samples.save_keys(inpth + ".dedup");
}


// void ImageDeduper::gen_all_binhash() {
//     cout << "gen bin hash with std::hash_t \n";

//     string inpth = src_paths[0];
//     sample_set samples;
//     samples.set_n_proc(n_proc);
//     samples.load_keys(inpth);
//     samples.gen_all_binhashes();
//     samples.save_samples_binhash(inpth + ".binhash");

// }


// void ImageDeduper::dedup_one_dataset_binhash() {
//     cout << "remove indentical by hash code \n";

//     string inpth = src_paths[0];
//     sample_set samples;

//     auto timer = Timer();
//     samples.set_n_proc(n_proc);
//     samples.load_samples_binhash(inpth);
//     samples.dedup_by_binhash();
//     samples.save_keys(inpth + ".dedup");
//     cout << "\t- time used: " << timer.time_duration() << endl;
// }


void ImageDeduper::process_pipeline() {

    auto timer = Timer();
    timer.start("global");

    string inpth = src_paths[0];

    // filter out low quality
    cout << "filter out images by rules" << endl;
    timer.start("step");
    string filt_savepth = inpth + ".filt";
    samples.load_keys(inpth);
    samples.filter_by_keys(img_keep_func);
    samples.save_keys(filt_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    // dedup by md5s
    cout << "gen all md5" << endl;
    timer.start("step");
    string md5_savepth = filt_savepth + ".md5";
    samples.gen_all_md5s();
    samples.save_samples_md5(md5_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "remove indentical by md5 code" << endl;
    timer.start("step");
    string md5_dedup_savepth = md5_savepth + ".dedup";
    samples.dedup_by_md5();
    samples.save_keys(md5_dedup_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    // dedup by dhash
    cout << "generate all dhash" << endl;
    timer.start("step");
    string dhash_savepth = md5_dedup_savepth + ".dhash";
    samples.gen_all_dhashes();
    samples.save_samples_dhash(dhash_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "dedup by dhash" << endl;
    timer.start("step");
    string dhash_dedup_savepth = dhash_savepth + ".dedup";
    samples.dedup_by_dhash(dhash_savepth);
    samples.save_keys(dhash_dedup_savepth);
    samples.save_samples_dhash(dhash_dedup_savepth + ".dhash");
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "time of whole pipeline: " << timer.time_duration("global") << endl;
}

