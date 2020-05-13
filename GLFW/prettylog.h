#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>

#if defined(__BIONIC__) && !defined(__clang__) && !defined(_GLIBCXX_USE_C99)
namespace std {
    using ::snprintf;
}
#elif defined(_MSC_VER) && _MSC_VER < 1900
# include <utility>
// minimal support is vs2013. vs2012 does not support variadic template parameters
namespace std {
    template<typename... Args> int snprintf(Args&&... args) { return _snprintf(std::forward<Args>(args)...);}
}
#endif

using namespace std;
static uint64_t get_time_and_string(char* s, size_t len, bool print_ms = true, const char* fmt = "%Y-%m-%d %T")
{
    using namespace chrono;
    const system_clock::time_point t = system_clock::now();
    const auto ms = duration_cast<milliseconds>(t - time_point_cast<seconds>(t));
    time_t now = system_clock::to_time_t(t); //time(0);
    if (print_ms) {
        char buf[22] = {};
        strftime(buf, sizeof(buf), fmt, localtime(&now));
        snprintf(s, len, "%s.%03d", buf, (int)ms.count());
    } else {
        strftime(s, len, fmt, localtime(&now));
    }
    return duration_cast<milliseconds>(t.time_since_epoch()).count();
}

static void print_log_msg(char level, const char* msg, FILE* f = stdout)
{
    char buf[32] = {};
    get_time_and_string(buf, sizeof(buf));
    std::stringstream t;
    t << this_thread::get_id();
    fprintf(f, "%c %s@%s: %s", level, buf, t.str().c_str(), msg);
    const int len = strlen(msg);
    if (len > 0 && msg[len-1] != '\n')
        fprintf(f, "\n");
    //fflush(f);
}