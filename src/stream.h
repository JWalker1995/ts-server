#pragma once

#include <deque>

#include "instant.h"
#include "chunk.h"
#include "event.h"
#include "writermanager.h"

class SubscriberConnection;

class Stream {
public:
    Stream(const std::string &key);

    const std::string &getKey() const { return key; }

    std::size_t getInitChunkId(Instant beginTime);

    std::size_t getNumChunks() const { return chunks.size(); }
    Chunk &getChunk(std::size_t chunkId) {
        assert(chunkId < chunks.size());
        return chunks[chunkId];
    }

    void tick(SubscriberConnection &conn);

    void publish(const char *data, std::size_t size);

    void unsubConnection(SubscriberConnection *conn);

private:
    std::string key;

    std::deque<Chunk> chunks;

    WriterManager writerManager;

    std::vector<SubscriberConnection *> realtimeSubscribers;
};
