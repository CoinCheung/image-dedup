
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>


using namespace std;


auto img_keep_func(const string& pth)->bool {
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

