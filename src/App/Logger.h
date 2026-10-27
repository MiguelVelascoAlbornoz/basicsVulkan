#pragma once
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
public:
    Logger() : f_(stdout) {}
    explicit Logger(FILE* f) : f_(f) {}

    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx_);
        fprintf(f_, "%s\n", msg.c_str());
        fflush(f_);
    }

    /** Concatenates every argument with operator<< and logs the result as a single line. */
    template <typename... Args>
    void log(const Args&... args) {
        std::ostringstream ss;
        (ss << ... << args);
        log(ss.str());
    }

private:
    FILE* f_;
    std::mutex mtx_;
};

inline Logger stdout_logger;
inline Logger stderr_logger{stderr};
