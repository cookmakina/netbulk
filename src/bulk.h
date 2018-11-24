#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <memory>

struct Bulk
{
    std::chrono::system_clock::time_point firstCmdTime;
    std::vector<std::string> commands;
};

std::ostream& operator <<(std::ostream& os, const Bulk &b) {
    os << "bulk: ";
    for (auto it = b.commands.begin(), iend = b.commands.end(); it != iend; ++it)
        os << (it == b.commands.begin() ? "" : ", ") << *it;;
    return os;
}

using BulkPtr = std::shared_ptr<Bulk>;
using BulkUniquePtr = std::unique_ptr<Bulk>;