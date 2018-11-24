#pragma once

using context_id_t = unsigned long long;

namespace {
    context_id_t InvalidContext = 0;
}

context_id_t connect(size_t bulkSize);
void receive(const void *bufPtr, size_t bufSize, context_id_t ctxId);
void disconnect(context_id_t);

void stopAll();