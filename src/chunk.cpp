#include "chunk.h"

#include "connection.h"
#include "readermanager.h"
#include "stream.h"

std::string Chunk::getFilename() const {
    return "chunk_" + std::to_string(beginTime.toUint64()) + "_" + std::to_string(endTime.toUint64()) + ".bin";
}

void Chunk::gc() {}

void Chunk::recvEvent(Event event) {
    events.push_back(event);
}

void Chunk::recvBestow(const char *data) {
    bestowed.push_back(data);
}

void Chunk::tick(Connection &conn) {
    switch (status) {
    case Status::Lazy:
        // Need to initialize the loading
        ReaderManager::getInstance().addReader(this);
        status = Status::Reading;

        // There won't be anything ready yet, so return
        return;

    case Status::Reading:
        emitEvents(conn);
        break;

    case Status::Done:
        emitEvents(conn);
        conn.nextChunkId++;
        break;
    }
}

void Chunk::emitEvents(Connection &conn) {
    while (conn.nextEventId < events.size()) {
        Event &e = events[conn.nextEventId++];
        if (e.time >= conn.beginTime) {
            if (e.time < conn.endTime) {
                conn.emit(std::string_view(e.data, e.size));
            } else {
                throw Stream::UnsubscribeException();
            }
        }
    }
}

void Chunk::freeMemory() {
    for (const char *mem : bestowed) {
        delete[] mem;
    }
    bestowed.clear();
}