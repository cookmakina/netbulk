#pragma once
#include <string>
#include <cstring>
#include "utils.h"
#include "bulk.h"

using BulkProcedure = std::function<void(const Bulk&)>;

class CommandBuffer
{
public:
    CommandBuffer(unsigned _bulkSize, BulkProcedure _bulkProc) :
        bulkSize(_bulkSize),
        bulkProc(std::move(_bulkProc)),
        connected(false) {
        bulk.commands.reserve(4);
    }

    ~CommandBuffer() {
    }

    void connect() {
        connected = true;
    }

    void disconnect() {
        connected = false;
        pushBulk();
    }

    void receiveText(const std::string &text) {
        receivedText += text;

        size_t lineStartIdx = 0;
        size_t lineEndIdx = receivedText.find('\n');
        size_t removeIdx = lineEndIdx;

        while (lineEndIdx != std::string::npos)
        {
            receivedText[lineEndIdx] = '\0';

            const char *lineStart = &receivedText[lineStartIdx];

            if (strstr(lineStart, "{")) {
                pushGroup();
            }
            else if (strstr(lineStart, "}")) {
                popGroup();
            }
            else {
                pushCommand(std::string(lineStart, (lineEndIdx-lineStartIdx)));
            }

            removeIdx = lineEndIdx;
            lineStartIdx = lineEndIdx + 1;
            lineEndIdx = receivedText.find('\n', lineStartIdx);
        }

        if (removeIdx != std::string::npos) {
            receivedText.erase(0, removeIdx+1);
        }
    }

    void pushCommand(const std::string &cmd) {
        if (cmd.empty())
            return;

        if (bulk.commands.empty())
            bulk.firstCmdTime = std::chrono::system_clock::now();

        bulk.commands.push_back(cmd);
        numTotalCommands++;
        numTotalLines++;

        if (bulk.commands.size() == bulkSize && groupDepth == 0) // if not in a group {} and we've got N commands, then send the bulk
            pushBulk();
    }

    void pushGroup() {
        if (groupDepth == 0 && !bulk.commands.empty())
            pushBulk();

        groupDepth++;
        numTotalLines++;
    }

    void popGroup() {
        groupDepth--;
        numTotalLines++;

        if (groupDepth == 0)
            pushBulk();

        else if (groupDepth < 0)
            throw std::runtime_error("Wrong sequence of parenthesis");
    }

    void pushBulk() {
        if (!numTotalCommands || bulk.commands.empty())
            return;

        numTotalBulks++;

        bulkProc(bulk);

        bulk.commands.clear();
    }

    void flush() {
        if (!bulk.commands.empty() && groupDepth == 0) // send bulk, but only if it's not within a group
            pushBulk();
    }

    unsigned long getLinesCount() const {
        return numTotalLines;
    }

    unsigned long getCommandsCount() const {
        return numTotalCommands;
    }

    unsigned long getBulksCount() const {
        return numTotalBulks;
    }

    bool isConnected() const {
        return connected;
    }

protected:
    unsigned bulkSize;
    bool connected;

    std::string receivedText;

    Bulk bulk;

    int groupDepth{ 0 };
    unsigned long numTotalCommands{ 0 };
    unsigned long numTotalLines{ 0 };
    unsigned long numTotalBulks{ 0 };

    BulkProcedure bulkProc;
};
