
#include <string>
#include <array>
#include <cstdint>
#include <vector>
#include <numeric>

#include <opencv2/opencv.hpp>
#include <openssl/evp.h>

#include "hash_func.h"
#include "utils.h"

/* 
 * Difference between phash and whash:
 * According to: https://www.ofcom.org.uk/siteassets/resources/documents/research-and-data/online-research/other/perceptual-hashing-technology.pdf?v=328806
 * whash perform better on images with fewer but more intense perturbations. For example, a big non-transparent logo overlaying on an image
 * phash perform better on images with less intense perturbation but covering large propotion on the image. For example, image-wide noise due to image compression or smoothing.
 *  */

vector<char> read_bin(const string& path) {
    // ifstream fin(path, ios::in);
    // stringstream ss;
    // fin >> ss.rdbuf();
    // fin.close();
    // return ss.str();

    ifstream fin(path, ios::in|ios::binary);
    CHECK(fin.is_open()) << "[ERROR] open for read fail: " << path 
            << ", errno: " << errno << endl;

    fin.seekg(0, fin.end); fin.clear();
    size_t len = fin.tellg();
    fin.seekg(0); fin.clear();

    vector<char> res; res.resize(len);
    fin.read(&res[0], len);
    fin.close();
    return res;
}


md5_t compute_md5(const vector<char>& s) {

    const EVP_MD *md = EVP_md5();
    uint32_t md_len = EVP_MD_size(md); // 16
    CHECK(md_len == nbytes_md5) << "md5_t type size is different from openssl md5 size" << endl;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();;
    EVP_DigestInit_ex(mdctx, md, nullptr);

    EVP_DigestUpdate(mdctx, &s[0], s.size());

    array<uint8_t, nbytes_md5> bytes; 
    EVP_DigestFinal_ex(mdctx, &bytes[0], &md_len);
    EVP_MD_CTX_free(mdctx);
    return md5_t::create_from_array_bytes(bytes);
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

    return binhash_t::create_from_size_t(hv);
}

dhash_t compute_dhash(const string& path, const uint16_t hw) {
    cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
    CHECK(!im.empty()) <<  "empty image: " << path << endl;

    cv::Mat resize;
    cv::resize(im, resize, cv::Size(hw + 1, hw + 1), 0, 0, cv::INTER_AREA);

    uint8_t *data = static_cast<uint8_t*>(resize.data);

    array<uint8_t, nbytes_dhash> bytes; 
    uint8_t byte = 0;
    uint32_t byte_ind = 0;
    uint8_t cnt = 0;
    uint16_t pos = 0;
    // traverse each row
    for (uint16_t h{0}; h < hw; ++h) {
        uint16_t ind1 = pos;
        uint16_t ind2 = ind1 + 1;

        for (uint16_t w{0}; w < hw; ++w) {
            byte = byte << 1;
            if (data[ind1] < data[ind2]) byte += 1;

            if (++cnt == 8) {
                bytes[byte_ind++] = byte;
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
    for (uint16_t w{0}; w < hw; ++w) {
        uint16_t ind1 = w;
        uint16_t ind2 = ind1 + hw + 1;

        for (uint16_t h{0}; h < hw; ++h) {
            byte = byte << 1;
            if (data[ind1] < data[ind2]) byte += 1;

            if (++cnt == 8) {
                bytes[byte_ind++] = byte;
                cnt = 0;
                byte = 0;
            }
            ind1 += hw + 1;
            ind2 += hw + 1;
        }
    }

    return dhash_t::create_from_array_bytes(bytes);
}


/* 
 * this is same as following python implementation(for 64-bit phash)
 * def phash_func(image, hash_size=8, highfreq_factor=4):
 *     img_size = hash_size * highfreq_factor
 *     image = cv2.resize(image, [img_size, img_size], interpolation=cv2.INTER_AREA)
 *     pixels = np.asarray(image).astype(np.float32)
 *     dct = cv2.dct(pixels)
 *     dct_low_freq = dct[:hash_size, :hash_size].ravel()
 *     med = np.median(dct_low_freq)
 *     hash = (dct_low_freq > med)
 *     return hash
 *  */
phash_t compute_phash(const string& path, const uint16_t hw) {
    cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
    CHECK(!im.empty()) <<  "empty image: " << path << endl;

    cv::Mat im_resize;
    cv::resize(im, im_resize, cv::Size(hw, hw), 0, 0, cv::INTER_AREA);

    im_resize.convertTo(im_resize, CV_32F);

    cv::Mat im_dct;
    cv::dct(im_resize, im_dct);

    uint16_t sub_hw = hw >> 2;
    uint32_t nbits = sub_hw * sub_hw;
    vector<float> dct_vec; dct_vec.reserve(nbits);

    for (uint16_t h{0}; h < sub_hw; ++h) {
        float* ptr = im_dct.ptr<float>(h);
        for (uint16_t w{0}; w < sub_hw; ++w) {
            dct_vec.push_back(ptr[w]);
        }
    }

    float thresh{0.};
    // use median
    vector<float> tmp(dct_vec.begin(), dct_vec.end());
    std::sort(tmp.begin(), tmp.end());
    thresh = (tmp[nbits >> 1] + tmp[(nbits >> 1) - 1]) / 2.;

    // use avg
    // thresh = std::accumulate(dct_vec.begin(), dct_vec.end(), 0.);
    // thresh /= static_cast<float>(nbits);

    array<uint8_t, nbytes_phash> bytes; 
    uint32_t byte_ind = 0;
    uint8_t byte{0};
    uint16_t cnt{0};
    for (float v : dct_vec) {
        byte = (byte << 1);
        if (v > thresh) {
            ++byte;
        }
        ++cnt;
        if (cnt == 8) {
            bytes[byte_ind++] = byte;
            cnt = 0;
            byte = 0;
        }
    }

    return phash_t::create_from_array_bytes(bytes);
}
