
#ifndef _UTILS_H_
#define _UTILS_H_


#include <iostream>
#include <string>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <functional>


using namespace std;


struct Timer {
public:

    Timer(const string& name);
    Timer();

    void start(const string& name);

    string time_duration(const string& name);
    string time_duration();

    template<typename F, typename... Args> 
    static auto run_func(F&& f, Args&&... args) -> decltype(f(args...)) {
        auto timer = Timer();

        using T = decltype(f(args...));
        if constexpr (is_same_v<T, void>) {
            std::invoke(f, std::forward(args)...); 
            cout << "\t- time used: " << timer.time_duration() << endl;
        } else {
            T res = std::invoke(f, std::forward(args)...);
            cout << "\t- time used: " << timer.time_duration() << endl;
            return res;
        }
    }

private:
    unordered_map<string, chrono::high_resolution_clock::time_point> m_starts;
};


struct check {
public:
    explicit check(bool success): m_success{ success } {}

    ~check() {
        if (!m_success) {
            cerr << m_ss.str();
            std::terminate();
        }
    }

    check(const check&) = delete;
    check(check&&) = default;
    check& operator=(const check&) = delete;
    check& operator=(check&&) = delete;

    template <typename T>
    check&& operator<<(const T& value) && {
        if (!m_success) {
            m_ss << value;
        }
        return std::move(*this);
    }

    check&& operator<<(std::ostream& (*manip)(std::ostream&)) && { // support endl/hex/setw
        if (!m_success) {
            manip(m_ss);
        }
        return std::move(*this);
    }


private:
    bool m_success{};
    std::stringstream m_ss{};
};

#define CHECK(predicate) (predicate) ? check{ true } : check{ false }

// #ifdef DEBUG
// #define CHECK(predicate) check{ (predicate) }
// #else
// #define CHECK(predicate) check{ true }
// #endif


#endif
