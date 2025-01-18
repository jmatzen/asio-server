#include <thread.hpp>
#include <atomic>

static std::atomic_int threadCounter;
thread_local int threadLocalId;


void jm::Thread::setLocalThreadId(int id) {
    threadLocalId = id;
}

int jm::Thread::thisThreadId() {
    return threadLocalId;
}

int jm::Thread::nextThreadId() {
    return threadCounter.fetch_add(1);
}