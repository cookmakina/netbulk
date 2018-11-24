#pragma once
#include <iostream>
#include <string>
#include "thread_bulk.h"


class ThreadConsoleLog: public ThreadBulk
{
public:
    explicit ThreadConsoleLog(const std::string& _id):
        ThreadBulk(_id)
    {
    }

    virtual void start() override {
        thrd = std::thread(&ThreadConsoleLog::threadProc, this);
    }

    void threadProc() {
        while (true) {
            BulkPtr bulk = waitGetBulk();
            if (stopFlag)
                break;
            if (!bulk)
                continue;

            std::cout << *bulk << std::endl;
        }

        printStats();
    }
};
