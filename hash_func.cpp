
#include <string>
#include <cstdint>
#include <vector>

#include <opencv2/opencv.hpp>
#include <openssl/evp.h>

#include "helper.h"
#include "samples.hpp"


md5_t compute_md5(const vector<char>& s) {

    const EVP_MD *md = EVP_md5();
    unsigned int md_len = EVP_MD_size(md); // 16

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();;
    EVP_DigestInit_ex(mdctx, md, nullptr);

    EVP_DigestUpdate(mdctx, &s[0], s.size());

    md5_t md_value;
    EVP_DigestFinal_ex(mdctx, &md_value[0], &md_len);
    EVP_MD_CTX_free(mdctx);
    return md_value;
}

md5_t compute_md5(const string& path) {
    auto bin = read_bin(path);
    return compute_md5(bin);
}

binhash_t compute_binbash(const string& path) {
    // cpp hash method(used in unordered_set), not necessarily md5, can be other hash methods(not for sure)
    auto bin = read_bin(path);
    size_t hv = std::hash<string>{}(
            // std::string(bin.begin(), bin.end())
            std::string(&bin[0], bin.size())
            );

    binhash_t binhash;
    std::memcpy(&binhash[0], &hv, sizeof(hv));

    return binhash;
}

dhash_t compute_dhash(const string& path, const uint16_t hw) {
    cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
    if (im.empty()) cout << "empty image: " << path << endl;

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

