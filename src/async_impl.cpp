#include <atomic>
#include <unordered_map>
#include <shared_mutex>
#include <cstring>
#include <memory>
#include <functional>
#include "async.h"
#include "command_buffer.h"
#include "thread_bulk.h"
#include "thread_file_log.h"
#include "thread_console_log.h"

enum class EParserCommand
{
    Connect,
    ReceiveText,
    Disconnect
};

struct ParserCommand
{
    context_id_t contextId{ InvalidContext };
    EParserCommand type;
    std::unique_ptr<std::string> payload;
    size_t bulkSize;
};

namespace {
    std::atomic<context_id_t> nextContext(1);

    std::unordered_map<context_id_t, CommandBuffer> contexts;

    std::queue<ParserCommand> parserQueue;
    std::mutex parserQueueMutex;
    std::condition_variable parserQueueEvent;
    std::atomic_bool parserStop{ false };

    std::unique_ptr<std::thread> parserThread;
    std::vector<std::unique_ptr<ThreadBulk>> fileThreads;
    std::unique_ptr<ThreadBulk> consoleThread;

    std::once_flag globalInited;
    std::once_flag globalFinished;
}

void parserThreadProc();

void globalInit() {
    const unsigned numFileThreads = 2;

    fileThreads.reserve((size_t)numFileThreads);

    for (int i = 0; i < numFileThreads; i++) {
        fileThreads.emplace_back(std::make_unique<ThreadFileLog>("File" + std::to_string(i+1)));
        fileThreads.back()->start();
    }

    consoleThread = std::make_unique<ThreadConsoleLog>("Console");
    consoleThread->start();

    parserThread = std::make_unique<std::thread>(&parserThreadProc);
}

context_id_t connect(size_t bulkSize)
{
    if (!bulkSize)
        return InvalidContext;

    std::call_once(globalInited, &globalInit);

    context_id_t id = nextContext.fetch_add(1);

    ParserCommand b;
    b.contextId = id;
    b.type = EParserCommand::Connect;
    b.bulkSize = bulkSize;

    {
        std::lock_guard<std::mutex> l(parserQueueMutex);
        parserQueue.push(std::move(b));
        parserQueueEvent.notify_one();
    }

    return id;
}

void receive(const void *pBuf, size_t bufSize, context_id_t ctxId) {
    if (!pBuf || bufSize == 0 || ctxId == InvalidContext)
        return;

    ParserCommand b;
    b.contextId = ctxId;
    b.type = EParserCommand::ReceiveText;
    b.payload = std::make_unique<std::string>(reinterpret_cast<const char *>(pBuf), bufSize);

    {
        std::lock_guard<std::mutex> l(parserQueueMutex);
        parserQueue.push(std::move(b));
        parserQueueEvent.notify_one();
    }
}

void disconnect(context_id_t ctxId) {
    if (ctxId == InvalidContext)
        return;

    ParserCommand b;
    b.contextId = ctxId;
    b.type = EParserCommand::Disconnect;

    {
        std::lock_guard<std::mutex> l(parserQueueMutex);
        parserQueue.push(std::move(b));
        parserQueueEvent.notify_one();
    }
}

void stopAll() {
    std::call_once(globalFinished, [](){
        parserStop = true;
        parserQueueEvent.notify_one(); // in case it's waiting for queue
        if (parserThread->joinable())
            parserThread->join();

        for (auto &t : fileThreads)
            t.reset();
        consoleThread.reset();
    });
}

////////////////////////////////////////////////////////
ParserCommand waitGetBuffer() {
    std::unique_lock<std::mutex> l(parserQueueMutex);

    if (!parserQueue.empty()) {
        ParserCommand cmd = std::move(parserQueue.front());
        parserQueue.pop();
        return cmd;
    }

    parserQueueEvent.wait(l);

    if (parserQueue.empty())
        return ParserCommand{};

    ParserCommand cmd = std::move(parserQueue.front());
    parserQueue.pop();
    return cmd;
}

void parserThreadProc()
{
    while (true) {
        if (parserStop)
            return;

        ParserCommand b = waitGetBuffer();

        if (parserStop)
            return;

        if (b.type == EParserCommand::Connect) {
            auto bulkProc = [](const Bulk& bulk) {
                size_t fileThrdIdx = rand() % fileThreads.size();
                if (fileThreads[fileThrdIdx])
                    fileThreads[fileThrdIdx]->sendBulk(bulk);
                if (consoleThread)
                    consoleThread->sendBulk(bulk);
            };

            CommandBuffer cmdbuf(b.bulkSize, bulkProc);
            cmdbuf.connect();

            contexts.emplace(b.contextId, cmdbuf);
            continue;
        }

        ///
        auto it = contexts.find(b.contextId);
        if (it == contexts.end()) {
            std::cout << "[Parser thread] Wrong context id: " << b.contextId << std::endl;
            continue;
        }

        CommandBuffer &cmdBuf = it->second;

        if (b.type == EParserCommand::ReceiveText) {
            if (cmdBuf.isConnected()) {
                cmdBuf.receiveText(std::move(*b.payload));
            }
        }
        else if (b.type == EParserCommand::Disconnect) {
            if (cmdBuf.isConnected()) {
                cmdBuf.disconnect();
                contexts.erase(it);
            }
        }
    }
}
