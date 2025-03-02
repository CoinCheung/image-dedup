
#ifndef _TOPOLOGY_H_
#define _TOPOLOGY_H_

#include <vector>
#include <string>
#include <iostream>


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

    static vector<string> inds_to_strings_vector(const vector<string>& keys, const vector<pair_t>& pairs_ind);

};


#endif
