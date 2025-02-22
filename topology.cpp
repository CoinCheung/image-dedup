
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

int triangle_t::get_i() {return i;}
int triangle_t::get_j() {return j;}

triangle_t::triangle_t(int pos) {
    int sum = 0;
    i = 0;
    while (true) {
        if (sum + i >= pos) break;
        sum += i + 1;
        ++i;
    }
    j = pos - sum;
}

void triangle_t::step(int n) { // move a step of n and update its coordinate
    while (j + n >= i + 1) {
        ++i;
        n -= i - j;
        j = 0;
    }
    j += n;
}

bool triangle_t::is_in_range(int n_levels) { // see if the current position is with a triagle with height n_levels
    if (i < 0 or j < 0) return false;
    if (i >= n_levels or j >= n_levels) return false;
    return true;
}

void triangle_t::print() { 
    cout << (*this) << endl;
}

ostream& operator<<(ostream& os, const triangle_t& tr) {
    os << "(" << tr.i << ", " << tr.j << ")";
    return os;
}


// a class of pair for coordinates

pair_t::pair_t(size_t i, size_t j, uint16_t diff): i(i), j(j), diff(diff) {}

void pair_t::print() {
    cout << (*this) << endl;
}

ostream& operator<<(ostream& os, const pair_t& p) {
    os << "i=" << p.i << ", j=" << p.j << ", diff=" << p.diff;
    return os;
}

