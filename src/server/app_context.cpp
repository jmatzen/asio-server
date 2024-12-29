#include <app_context.hpp>
#include <exception>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread.hpp>

using namespace jm;

static std::unique_ptr<AppContext> g_appContext;

AppContext::AppContext() {}

AppContext::~AppContext() {}

AppContext &AppContext::getContext() {
    if (!g_appContext) {
        throw std::runtime_error("the application context does not exist.");
    }
    return *g_appContext;
}

void AppContext::initialize() {
    g_appContext = std::make_unique<AppContext>();
    g_appContext->getIoContext().post([]() {
        spdlog::info("initialized");
    });
    g_appContext->start();
}

void AppContext::start() {
    const int numThreads = std::thread::hardware_concurrency();
    threads_.reserve(numThreads);
    for (int i = 0; i != numThreads; ++i) {
        auto thread = Thread([this]() { ioContext_.get().run(); });
        threads_.push_back(std::move(thread));
    }
}
void AppContext::waitForAppExit() {
    for (auto &thread : threads_) {
        if (thread->joinable()) {
            thread->join();
            spdlog::info("thread {} joined", (intptr_t)thread.id());
        }
    }
}

