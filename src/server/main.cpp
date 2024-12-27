#include <acceptor.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread.hpp>
#include <thread>
#include <vector>

using namespace jm;

void start(IoContext &context) {
    auto acceptor = std::make_shared<Acceptor>(context, 8080);

    // set up a timer
    auto timer = context.fixedDelay(boost::posix_time::millisec(1000), []() {
        spdlog::info("timer thread={}", Thread::thisThreadId());
        throw std::runtime_error("failed.");
    });
}

int main(int argc, char **argv) {
    auto io_context = std::make_shared<IoContext>();
    start(*io_context);

    const int numThreads = std::thread::hardware_concurrency();
    std::vector<Thread> threads;
    for (int i = 0; i != numThreads; ++i) {
        auto thread =
            Thread([&io_context]() { io_context.get()->get().run(); });
        threads.push_back(std::move(thread));
    }
    for (auto &thread : threads) {
        if (thread->joinable()) {
            thread->join();
            spdlog::info("thread {} joined", (intptr_t)thread.id());
        }
    }
    spdlog::info("end!");
    return 0;
}
