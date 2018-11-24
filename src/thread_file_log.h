#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "thread_bulk.h"
#include "utils.h"

class ThreadFileLog: public ThreadBulk
{
public:
    explicit ThreadFileLog(const std::string& _id):
        ThreadBulk(_id)
    {
    }

    virtual void start() override {
        thrd = std::thread(&ThreadFileLog::threadProc, this);
    }

    void threadProc() {
        while (true) {
            BulkPtr bulk = waitGetBulk();
            if (stopFlag)
                break;
            if (!bulk)
                continue;

            // send to a file
            std::string filename = "bulk";
            filename += timeToString(bulk->firstCmdTime);
            filename += '_';
            filename += std::to_string(rand() % 1000);
            filename += ".log";

            std::ofstream f(filename);
            if (!f.is_open()) {
                std::cout << "Can't open log file for write: " << filename << std::endl;
                continue;
            }

            f << *bulk;

            f.close();
        }

        printStats();
    }
};
