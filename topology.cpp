
#include <vector>
#include <string>
#include <iostream>

#include "topology.h"
#include "big_int.hpp"


//////////////////////////////
/* 
 * topology class for comparing pairs
 */
//////////////////////////////

using namespace std;

// a triangle shape class, used to fetch coordinates from a global index

int triangle_t::get_i() {return m_i;}
int triangle_t::get_j() {return m_j;}

triangle_t::triangle_t(int pos) {
    int sum = 0;
    m_i = 0;
    while (true) {
        if (sum + m_i >= pos) break;
        sum += m_i + 1;
        ++m_i;
    }
    m_j = pos - sum;
}

void triangle_t::step(int n) { // move a step of n and update its coordinate
    while (m_j + n >= m_i + 1) {
        ++m_i;
        n -= m_i - m_j;
        m_j = 0;
    }
    m_j += n;
}

bool triangle_t::is_in_range(int n_levels) { // see if the current position is with a triagle with height n_levels
    if (m_i < 0 or m_j < 0) return false;
    if (m_i >= n_levels or m_j >= n_levels) return false;
    return true;
}

void triangle_t::print() { 
    cout << (*this) << endl;
}

ostream& operator<<(ostream& os, const triangle_t& tr) {
    os << "(" << tr.m_i << ", " << tr.m_j << ")";
    return os;
}


// a class of pair for coordinates

pair_t::pair_t(size_t i, size_t j, uint16_t diff): m_i(i), m_j(j), m_diff(diff) {}

void pair_t::print() {
    cout << (*this) << endl;
}

ostream& operator<<(ostream& os, const pair_t& p) {
    os << "i=" << p.m_i << ", j=" << p.m_j << ", diff=" << p.m_diff;
    return os;
}


vector<string> pair_t::inds_to_strings_vector(const vector<string>& keys, 
        const vector<pair_t>& pairs_ind) {
    vector<string> res;
    stringstream ss;
    for (auto &pair : pairs_ind) {
        ss.str(""); ss.clear();
        ss << keys[pair.m_i] << ","
            << keys[pair.m_j]
            << "," << pair.m_diff; 
        res.emplace_back(ss.str());
    }
    return res;
}
