
#ifndef _HELPER_H_
#define _HELPER_H_

#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include <sstream>
#include <vector>

using namespace std;


vector<string> read_lines(const string&);

vector<char> read_bin(const string&);

void print_string_as_hex(string& str);

template<typename T>
void save_result(const vector<T>& res, const string& savepth) {
    size_t n_samples = res.size();
    stringstream ss;
    for (size_t i{0}; i < n_samples; ++i) {
        ss << res[i];
        if (i < n_samples - 1) ss << endl;
    }

    ss.clear(); ss.seekg(0, ios::beg); 
    ofstream fout(savepth, ios::out);
    if (!fout.is_open()) {
        cerr << "[ERROR] open for write fail: " << savepth 
            << ", errno: " << errno << endl;
    }
    ss >> fout.rdbuf(); fout.close();
}


#endif

