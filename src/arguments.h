#pragma once
#include <vector>
#include <string>
#include <cstdlib>

class ProgramArguments
{
    std::vector<std::string> args;
    std::string modulePath;

public:
    ProgramArguments(int argc, const char **argv)
    {
        if (argc < 1)
            return;

        modulePath = argv[0];

        args.reserve((size_t)(argc-1));
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg.empty())
                continue;

            if (arg[0] == '-')
            {
                if (arg.size() > 1 && arg[1] == '-')
                    arg = arg.substr(2);
                else
                    arg = arg.substr(1);
            }

            args.push_back(std::move(arg));
        }
    }

    const std::string& operator[](size_t ind) const
    {
        return args[ind];
    }

    int getInt(size_t ind) const {
        char *pend = nullptr;
        int number = (int)strtol(args[ind].c_str(), &pend, 10);
        return number;
    }

    size_t count() const
    {
        return args.size();
    }

    bool hasAny() const
    {
        return !args.empty();
    }

    bool contains(const char *argName) const
    {
        for (auto i = std::begin(args), ie = std::end(args); i != ie; ++i)
        {
            if (*i == argName)
                return true;
        }
        return false;
    }

    // returns argument that goes after the given, or empty string
    std::string after(const char *argName) const
    {
        std::string result;
        for (auto i = std::begin(args), ie = std::end(args); i != ie; ++i)
        {
            if (*i == argName)
            {
                if (i != ie-1) // if it's not the last argument
                    result = *(i + 1);
                break;
            }
        }
        return result;
    }

    const std::string& getModulePath() const // UTF-8
    {
        return modulePath;
    }
};