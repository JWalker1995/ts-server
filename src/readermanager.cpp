#include "readermanager.h"

#include "chunk.h"

#include "filereader.h"
#include "decompressor.h"
#include "decoder.h"
#include "queuepublisher.h"

void ReaderManager::addReader(Chunk *chunk) {
    readers.emplace_back(chunk, [](Reader *reader) {
        FileReader<Decompressor<Decoder<QueuePublisher>>> pipe(std::ref(reader->queue));
        pipe.read(reader->chunk->getFilename(), [reader](){return reader->shouldRun.test_and_set();});
    });
}

ReaderManager::~ReaderManager() {
    for (Reader &reader : readers) {
        reader.shouldRun.clear();
    }
}

ReaderManager::Reader::~Reader() {
    if (chunk) {
        assert(thread.joinable());
        thread.join();
    }
}

void ReaderManager::tick() {
    for (Reader &reader : readers) {
        if (reader.chunk == 0) { continue; }

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        QueueMessage msg;
        while (reader.queue.try_dequeue(msg)) {
            if (std::holds_alternative<Event>(msg)) {
                reader.chunk->onEvent(std::get<Event>(msg));
            } else if (std::holds_alternative<Bestow<std::unique_ptr<char[]>>>(msg)) {
                reader.chunk->onBestow(std::move(std::get<Bestow<std::unique_ptr<char[]>>>(msg).mem));
            } else if (std::holds_alternative<Bestow<std::shared_ptr<char>>>(msg)) {
                reader.chunk->onBestow(std::move(std::get<Bestow<std::shared_ptr<char>>>(msg).mem));
            } else if (std::holds_alternative<Bestow<std::vector<char>>>(msg)) {
                reader.chunk->onBestow(std::move(std::get<Bestow<std::vector<char>>>(msg).mem));
            } else if (std::holds_alternative<Yield>(msg)) {
                break; // Don't join immediately to give the thread some time to clean up
            } else if (std::holds_alternative<Join>(msg)) {
                reader.chunk->onEnd();

                reader.thread.join();
                reader.chunk = 0; // Signals that this reader can be deleted

                break;
            } else {
                assert(false);
            }

            if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(1)) {
                break;
            }
        }
    }

    while (!readers.empty() && readers.front().chunk == 0) {
        readers.pop_front();
    }
    while (!readers.empty() && readers.back().chunk == 0) {
        readers.pop_back();
    }
}
