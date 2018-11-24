#pragma once
#include <cassert>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include "bulk.h"

class ThreadBulk {
public:
    explicit ThreadBulk(std::string _id) :
        id(std::move(_id)),
        mainThreadId(std::this_thread::get_id())
    {
    }

    virtual ~ThreadBulk() {
        stop();
        join();
    }

    void sendBulk(const Bulk& b) {
        BulkUniquePtr bcopy = std::make_unique<Bulk>(b);
        {
            std::lock_guard<std::mutex> g(bulkQueueMutex);
            bulkQueue.push(std::move(bcopy));
        }
        bulkReadyEvent.notify_one();
    }

    BulkUniquePtr consumeBulkFromQueue() { // called from the thread
        if (bulkQueue.empty())
            return nullptr;

        BulkUniquePtr bulk = std::move(bulkQueue.front());
        bulkQueue.pop();

        numBulks++;
        numCmds += bulk->commands.size();
        return bulk;
    }

    BulkUniquePtr waitGetBulk() { // called from the thread
        if (stopFlag) {
            return nullptr;
        }

        std::unique_lock<std::mutex> lk(bulkQueueMutex);
        if (!bulkQueue.empty()) {
            return consumeBulkFromQueue();
        }

        bulkReadyEvent.wait(lk);
        if (stopFlag)
           return nullptr;

        return consumeBulkFromQueue();
    }

    virtual void start() {
    }

    void stop() {
        stopFlag = true;
        bulkReadyEvent.notify_one();
    }

    void join() {
        if (thrd.joinable())
            thrd.join();
    }

protected:
    void printStats() {
        std::cout << "Thread " << id << ": " << numBulks << " blocks, " << numCmds << " commands, " << bulkQueue.size()
                  << " left in queue" << std::endl;
    }

    std::thread thrd;
    std::queue<BulkUniquePtr> bulkQueue;
    std::mutex bulkQueueMutex;
    std::condition_variable bulkReadyEvent;

    std::string id;

    unsigned long numBulks{ 0 };
    unsigned long numCmds{ 0 };

    std::atomic_bool stopFlag{ false };
    std::thread::id mainThreadId;
};

#undef REQUIRE_MAIN_THREAD
#undef REQUIRE_NON_MAIN_THREAD
