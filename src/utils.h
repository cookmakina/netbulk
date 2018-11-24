#pragma once
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>

std::string timeToString(const std::chrono::system_clock::time_point & t) {
    auto nanos = t.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(nanos);

    return std::to_string(millis.count());
}

void stripSpaces(std::string &line) {
    if (line.empty())
        return;
    size_t p1 = line.find_first_not_of(' ');
    if (p1 == std::string::npos) {
        line = "";
        return;
    }
    size_t p2 = line.find_last_not_of(' ');

    line = line.substr(p1, p2-p1+1);
}