

#include <iomanip>
#include <iostream>

#include "utils.h"


using namespace std;


//////////////////////////////
/* 
 * For Timer class
 */
//////////////////////////////

Timer::Timer(const string& name) {
    start(name);
}


Timer::Timer(): Timer("default") {}


void Timer::start(const string& name) {
    starts[name] = chrono::high_resolution_clock::now();
}


string Timer::time_duration(const string& name) {
    using namespace std::chrono;

    auto dura = chrono::high_resolution_clock::now() - starts[name];

    auto hour = duration_cast<hours>(dura);
    dura -= hour;

    auto minu = duration_cast<minutes>(dura);
    dura -= minu;

    auto sec = duration_cast<seconds>(dura);
    dura -= sec;

    stringstream ss;
    ss << std::setfill('0') 
       << std::setw(2) << hour.count() << ":" 
       << std::setw(2) << minu.count() << ":" 
       << std::setw(2) << sec.count();
    return ss.str();
}


string Timer::time_duration() {
    return time_duration("default");
}


