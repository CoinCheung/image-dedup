
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
    m_cmd = argv[1];
    auto get_n_proc = [](const char* s) {
        size_t n_proc = static_cast<size_t>(std::stoi(string(s)));
        return n_proc;
    };

    if (m_cmd == "filter") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::filter_out_images, this);

    } else if (m_cmd == "gen_md5") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::gen_all_md5, this);

    } else if (m_cmd == "dedup_md5") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::dedup_one_dataset_md5, this);

    } else if (m_cmd == "gen_dhash") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::gen_all_dhash, this);

    } else if (m_cmd == "dedup_dhash") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::dedup_one_dataset_dhash, this);

    } else if (m_cmd == "merge_dhash") {
        for (int i{3}; i < argc - 1; ++i) {
            m_src_paths.push_back(argv[i]);
        }
        m_dst_paths.push_back(argv[argc - 1]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::merge_datasets_dhash, this);

    } else if (m_cmd == "remain_dhash") {
        m_src_paths.push_back(argv[3]);
        m_dst_paths.push_back(argv[4]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::remain_datasets_dhash, this);

    } else if (m_cmd == "gen_phash") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::gen_all_phash, this);

    } else if (m_cmd == "dedup_phash") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::dedup_one_dataset_phash, this);

    } else if (m_cmd == "merge_phash") {
        for (int i{3}; i < argc - 1; ++i) {
            m_src_paths.push_back(argv[i]);
        }
        m_dst_paths.push_back(argv[argc - 1]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::merge_datasets_phash, this);

    } else if (m_cmd == "remain_phash") {
        m_src_paths.push_back(argv[3]);
        m_dst_paths.push_back(argv[4]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::remain_datasets_phash, this);

    } else if (m_cmd == "pipeline") {
        m_src_paths.push_back(argv[3]);
        m_funcs[m_cmd] = std::bind(&ImageDeduper::process_pipeline, this);
    }

    size_t n_proc = get_n_proc(argv[2]);
    m_samples.set_n_proc(n_proc);
}


void ImageDeduper::run_cmd() {
     
    if (m_cmd == "pipeline") {
        process_pipeline();
    } else {
        Timer::run_func(m_funcs[m_cmd]);
    }
}


void ImageDeduper::filter_out_images() {
    /// filter out images by some rule
    cout << "filter out images by rules" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_keys(inpth);
    m_samples.filter_by_keys(img_keep_func);
    m_samples.save_keys(inpth + ".filt");
}

//// dhash relevant

void ImageDeduper::gen_all_dhash() {
    /// generate dhashes given paths of images
    cout << "generate all dhash" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_keys(inpth);
    m_samples.gen_all_dhashes();
    m_samples.save_samples_dhash(inpth + ".dhash");
}


void ImageDeduper::dedup_one_dataset_dhash() {
    /// dedup within one dataset using dhash
    cout << "dedup by dhash" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_samples_dhash(inpth); // TODO: here if samples is not empty, need to check whether inpth missing keys
    m_samples.dedup_by_dhash(inpth);
    m_samples.save_samples_dhash(inpth + ".dedup.dhash");
    m_samples.save_keys(inpth + ".dedup");
}


void ImageDeduper::merge_datasets_dhash() {
    /// merge multi datasets together, each of which has already been deduplicated
    cout << "merge multiple deduped datasets by dhash" << endl;

    vector<string> inpths = m_src_paths;
    string savepth = m_dst_paths[0];

    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        sample_set samplesi;
        samplesi.load_samples_dhash(inpths[i]);
        m_samples.merge_other_dhash(samplesi);
    }
    m_samples.save_keys(savepth);
}


void ImageDeduper::remain_datasets_dhash() {
    /// drop those paths in src_path which duplicates with some path in dst_path
    
    string src_path = m_src_paths[0];
    string dst_path = m_dst_paths[0];

    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;

    m_samples.load_samples_dhash(src_path);
    sample_set dst_samples;
    dst_samples.load_samples_dhash(dst_path);

    m_samples.drop_exists_by_dhash(dst_samples);
    m_samples.save_keys(src_path + ".rem");
}


//// phash relevant

void ImageDeduper::gen_all_phash() {
    /// generate phashes given paths of images
    cout << "generate all phash" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_keys(inpth);
    m_samples.gen_all_phashes();
    m_samples.save_samples_phash(inpth + ".phash");
}


void ImageDeduper::dedup_one_dataset_phash() {
    /// dedup within one dataset using phash
    cout << "dedup by phash" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_samples_phash(inpth); // TODO: here if samples is not empty, need to check whether inpth missing keys
    m_samples.dedup_by_phash(inpth);
    m_samples.save_samples_phash(inpth + ".dedup.phash");
    m_samples.save_keys(inpth + ".dedup");
}


void ImageDeduper::merge_datasets_phash() {
    /// merge multi datasets together, each of which has already been deduplicated
    cout << "merge multiple deduped datasets by phash" << endl;

    vector<string> inpths = m_src_paths;
    string savepth = m_dst_paths[0];

    size_t n = inpths.size();
    for (size_t i{0}; i < n; ++i) {
        sample_set samplesi;
        samplesi.load_samples_phash(inpths[i]);
        m_samples.merge_other_phash(samplesi);
    }
    m_samples.save_keys(savepth);
}


void ImageDeduper::remain_datasets_phash() {
    /// drop those paths in src_path which duplicates with some path in dst_path
    
    string src_path = m_src_paths[0];
    string dst_path = m_dst_paths[0];

    cout << "obtain remaining images in: [" << src_path 
         << "], after dropping those duplicates with images in: [" << dst_path << "]" << endl;

    m_samples.load_samples_phash(src_path);
    sample_set dst_samples;
    dst_samples.load_samples_phash(dst_path);

    m_samples.drop_exists_by_phash(dst_samples);
    m_samples.save_keys(src_path + ".rem");
}


//// phash relevant

void ImageDeduper::gen_all_md5() {

    cout << "gen all md5" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_keys(inpth);
    m_samples.gen_all_md5s();
    m_samples.save_samples_md5(inpth + ".md5");
}


void ImageDeduper::dedup_one_dataset_md5() {
    cout << "remove indentical by md5 code" << endl;

    string inpth = m_src_paths[0];

    m_samples.load_samples_md5(inpth);
    m_samples.dedup_by_md5();
    m_samples.save_keys(inpth + ".dedup");
}


// void ImageDeduper::gen_all_binhash() {
//     cout << "gen bin hash with std::hash_t \n";

//     string inpth = m_src_paths[0];
//     sample_set samples;
//     samples.set_n_proc(n_proc);
//     samples.load_keys(inpth);
//     samples.gen_all_binhashes();
//     samples.save_samples_binhash(inpth + ".binhash");

// }


// void ImageDeduper::dedup_one_dataset_binhash() {
//     cout << "remove indentical by hash code \n";

//     string inpth = m_src_paths[0];
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

    string inpth = m_src_paths[0];

    // filter out low quality
    cout << "filter out images by rules" << endl;
    timer.start("step");
    string filt_savepth = inpth + ".filt";
    m_samples.load_keys(inpth);
    m_samples.filter_by_keys(img_keep_func);
    m_samples.save_keys(filt_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    // dedup by md5s
    cout << "gen all md5" << endl;
    timer.start("step");
    string md5_savepth = filt_savepth + ".md5";
    m_samples.gen_all_md5s();
    m_samples.save_samples_md5(md5_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "remove indentical by md5 code" << endl;
    timer.start("step");
    string md5_dedup_savepth = md5_savepth + ".dedup";
    m_samples.dedup_by_md5();
    m_samples.save_keys(md5_dedup_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    // dedup by phash
    cout << "generate all phash" << endl;
    timer.start("step");
    string phash_savepth = md5_dedup_savepth + ".phash";
    m_samples.gen_all_phashes();
    m_samples.save_samples_phash(phash_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "dedup by phash" << endl;
    timer.start("step");
    string phash_dedup_savepth = phash_savepth + ".dedup";
    m_samples.dedup_by_phash(phash_savepth);
    m_samples.save_keys(phash_dedup_savepth);
    m_samples.save_samples_phash(phash_dedup_savepth + ".phash");
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    // dedup by dhash
    cout << "generate all dhash" << endl;
    timer.start("step");
    // string dhash_savepth = md5_dedup_savepth + ".dhash";
    string dhash_savepth = phash_dedup_savepth + ".dhash";
    m_samples.gen_all_dhashes();
    m_samples.save_samples_dhash(dhash_savepth);
    cout << "\t- time used: " << timer.time_duration("step") << endl;

    cout << "dedup by dhash" << endl;
    timer.start("step");
    string dhash_dedup_savepth = dhash_savepth + ".dedup";
    m_samples.dedup_by_dhash(dhash_savepth);
    m_samples.save_keys(dhash_dedup_savepth);
    m_samples.save_samples_dhash(dhash_dedup_savepth + ".dhash");
    cout << "\t- time used: " << timer.time_duration("step") << endl;


    cout << "time of whole pipeline: " << timer.time_duration("global") << endl;
}

