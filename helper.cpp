
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>


using namespace std;

vector<string> read_lines(const string& inpth) {
    ifstream fin(inpth, ios::in);
    if (!fin.is_open()) {
        cerr << "[ERROR] open for read fail: " << inpth 
            << ", errno: " << errno << endl;
    }
    stringstream ss;
    fin >> ss.rdbuf();
    fin.close();

    string buf;

    ss.clear();ss.seekg(0, ios::beg);
    vector<string> lines;
    while (getline(ss, buf)) {
        lines.push_back(buf);
    }
    return lines;
}


vector<char> read_bin(const string& path) {
    // ifstream fin(path, ios::in);
    // stringstream ss;
    // fin >> ss.rdbuf();
    // fin.close();
    // return ss.str();

    ifstream fin(path, ios::in|ios::binary);
    if (!fin.is_open()) {
        cerr << "[ERROR] open for read fail: " << path 
            << ", errno: " << errno << endl;
    }

    fin.seekg(0, fin.end); fin.clear();
    size_t len = fin.tellg();
    fin.seekg(0); fin.clear();

    vector<char> res; res.resize(len);
    fin.read(&res[0], len);
    fin.close();
    return res;
}


void print_string_as_hex(string& str) {
    cout << std::hex;
    for (char c : str) {
        cout << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
    }
}

