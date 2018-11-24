#include <iostream>
#include <algorithm>
#include <thread>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include "utils.h"
#include "async.h"

std::atomic_bool stopSig(false);

void stopper()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stopSig = true;
}

int main(int argc, char *argv[]) {
    std::thread t(&stopper);
    t.detach();

    const unsigned startBulkSize = 2;

    std::string commands;
    commands.reserve(1000);

    std::vector<context_id_t> contexts;
    contexts.reserve(200);
    contexts.push_back(connect(startBulkSize));
    contexts.push_back(connect(startBulkSize));

    while (!stopSig)
    {
        int r = rand() % 100;
        if (r < 10) {
            // open a context with random bulkSize
            int bulkSize = 1 + rand() % 5;
            if (rand() % 10 < 3)
                bulkSize += 1 + rand() % 12;

            context_id_t id = connect(bulkSize);
            if (!id)
                continue;

            contexts.push_back(id);
        }
        else if (r < 21) {
            if (contexts.empty())
                continue;

            // close a random context
            size_t idx = rand() % contexts.size();
            context_id_t id = contexts[idx];

            disconnect(id);
            contexts.erase(contexts.begin() + idx);
        }
        else if (r < 100) {
            if (contexts.empty())
                continue;

            // send random cmds set to a random
            int numCommands = rand() % 7;
            if (rand() % 10 < 3)
                numCommands += rand() % 14;

            commands.clear();
            for (int i = 0; i < numCommands; i++)
            {
                for (int j = 0; j < 2; j++)
                    commands += ('a' + rand() % 25);
                commands += "\n";
            }

            size_t idx = rand() % contexts.size();
            context_id_t id = contexts[idx];

            receive(commands.data(), commands.size(), id);
        }

        if (rand() % 25 == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    for (auto id : contexts) {
        disconnect(id);
    }

    stopAll();

    return EXIT_SUCCESS;
}
