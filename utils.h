
#ifndef _UTILS_H_
#define _UTILS_H_


#include <iostream>
#include <string>
#include <chrono>
#include <sstream>
#include <unordered_map>


using namespace std;


struct Timer {
public:
    Timer(const string& name);
    Timer();

    void start(const string& name);

    string time_duration(const string& name);
    string time_duration();

private:
    unordered_map<string, chrono::high_resolution_clock::time_point> starts;
};


struct CHECK {
public:
    explicit CHECK(bool success);

    ~CHECK();

    CHECK(const CHECK&) = delete;
    CHECK(CHECK&&) = delete;
    CHECK& operator=(const CHECK&) = delete;
    CHECK& operator=(CHECK&&) = delete;

    template <typename T>
    CHECK&& operator<<(const T& value) && {
        if (!success) {
            s << value;
        }
        return std::move(*this);
    }

    CHECK&& operator<<(std::ostream& (*)(std::ostream&)) &&; // support endl/hex/setw


private:
    bool success{};
    std::stringstream s{};
};




// #ifdef DEBUG
// #define CHECK(predicate) check{ (predicate) }
// #else
// #define CHECK(predicate) check{ true }
// #endif


#endif
