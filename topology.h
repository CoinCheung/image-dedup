
#ifndef _TOPOLOGY_H_
#define _TOPOLOGY_H_

#include <vector>
#include <string>
#include <iostream>

#include "samples.hpp"

using namespace std;


struct triangle_t {
private:
    int i{-1};
    int j{-1};

public:
    int get_i();
    int get_j();

    triangle_t(int pos); 

    void step(int n);

    bool is_in_range(int n_levels); 

    void print(); 

    friend ostream& operator<<(ostream& os, const triangle_t& tr);
};


struct pair_t {
    size_t i;
    size_t j;
    uint16_t diff;
    pair_t(size_t i, size_t j, uint16_t diff);
    
    void print(); 

    friend ostream& operator<<(ostream& os, const pair_t& p);

    template<typename hash_t>
    static vector<string> inds_to_strings_vector(const sample_set<hash_t>& samples, 
            const vector<pair_t>& pairs_ind) {
        vector<string> res;
        stringstream ss;
        for (auto &pair : pairs_ind) {
            ss.str(""); ss.clear();
            ss << samples[pair.i].key << ","
                << samples[pair.j].key 
                << "," << pair.diff; 
            res.emplace_back(ss.str());
        }
        return res;
    }

};


#endif
