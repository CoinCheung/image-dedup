
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <chrono>

#include "samples.h"
#include "image_deduper.h"
#include "image_filter.h"


using namespace std;


struct Timer{
public:
    Timer(const string& name) {
        starts[name] = chrono::high_resolution_clock::now();
    }

    Timer(): Timer("default") {}

    void start(const string& name) {
        starts[name] = chrono::high_resolution_clock::now();
    }

    string time_duration(const string& name) {
        using namespace std::chrono;

        auto dura = chrono::high_resolution_clock::now() - starts[name];

        auto hour = duration_cast<hours>(dura);
        dura -= hour;

        auto minu = duration_cast<minutes>(dura);
        dura -= minu;

        auto sec = duration_cast<seconds>(dura);
        dura -= sec;

        stringstream ss;
        ss << std::setfill('0') 
           << std::setw(2) << hour.count() << ":" 
           << std::setw(2) << minu.count() << ":" 
           << std::setw(2) << sec.count();
        return ss.str();
    }
    
    string time_duration() {
        return time_duration("default");
    }

private:
    unordered_map<string, chrono::high_resolution_clock::time_point> starts;
};



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

    } else if (cmd == "pipeline") {
        n_proc = get_n_proc(argv[2]);
        src_paths.push_back(argv[3]);
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
    } else if (cmd == "pipeline") {
        process_pipeline();
    }
}


void ImageDeduper::filter_out_images() {
    /// filter out images by some rule
    cout << "filter out images by rules" << endl;

    string inpth = src_paths[0];
    sample_set samples;

    auto timer = Timer();
    samples.set_n_proc(n_proc);
    samples.load_keys(inpth);
    samples.filter_by_keys(img_keep_func);
    samples.save_keys(inpth + ".filt");
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::gen_all_dhash() {
    /// generate dhashes given paths of images
    cout << "generate dhash" << endl;

    string inpth = src_paths[0];
    sample_set samples;

    auto timer = Timer();
    samples.set_n_proc(n_proc);
    samples.load_keys(inpth);
    samples.gen_all_dhashes();
    samples.save_samples_dhash(inpth + ".dhash");
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::dedup_one_dataset_dhash() {
    /// dedup within one dataset using dhash
    cout << "dedup by dhash" << endl;

    string inpth = src_paths[0];
    sample_set samples;

    auto timer = Timer();
    samples.set_n_proc(n_proc);
    samples.load_samples_dhash(inpth); // TODO: here if samples is not empty, need to check whether inpth missing keys
    samples.dedup_by_dhash();
    samples.save_samples_dhash(inpth + ".dedup.dhash");
    samples.save_keys(inpth + ".dedup");
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::merge_datasets_dhash() {
    /// merge multi datasets together, each of which has already been deduplicated
    cout << "merge multiple deduped datasets by dhash" << endl;

    vector<string> inpths = src_paths;
    string savepth = dst_paths[0];
    sample_set deduped;
    deduped.set_n_proc(n_proc);

    auto timer = Timer();
    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        sample_set samplesi;
        samplesi.load_samples_dhash(inpths[i]);
        deduped.merge_other_dhash(samplesi);
    }
    deduped.save_keys(savepth);
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::remain_datasets_dhash() {
    /// drop those paths in src_path which duplicates with some path in dst_path
    
    auto timer = Timer();
    string src_path = src_paths[0];
    string dst_path = dst_paths[0];

    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;

    sample_set src_samples;
    sample_set dst_samples;
    src_samples.set_n_proc(n_proc);
    src_samples.load_samples_dhash(src_path);
    dst_samples.load_samples_dhash(dst_path);

    src_samples.drop_exists_by_dhash(dst_samples);
    src_samples.save_keys(src_path + ".rem");
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::gen_all_md5() {

    cout << "gen all md5" << endl;

    string inpth = src_paths[0];
    sample_set samples;

    auto timer = Timer();
    samples.set_n_proc(n_proc);
    samples.load_keys(inpth);
    samples.gen_all_md5s();
    samples.save_samples_md5(inpth + ".md5");
    cout << "\t- time used: " << timer.time_duration() << endl;
}


void ImageDeduper::dedup_one_dataset_md5() {
    cout << "remove indentical by md5 code" << endl;

    string inpth = src_paths[0];
    sample_set samples;

    auto timer = Timer();
    samples.set_n_proc(n_proc);
    samples.load_samples_md5(inpth);
    samples.dedup_by_md5();
    samples.save_keys(inpth + ".dedup");
    cout << "\t- time used: " << timer.time_duration() << endl;
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
    filter_out_images();

    src_paths[0] = src_paths[0] + ".filt";
    gen_all_md5();

    src_paths[0] = src_paths[0] + ".md5";
    dedup_one_dataset_md5();

    src_paths[0] = src_paths[0] + ".dedup";
    gen_all_dhash();

    src_paths[0] = src_paths[0] + ".dhash";
    dedup_one_dataset_dhash();

    cout << "\ttime of whole pipeline: " << timer.time_duration() << endl;
}

