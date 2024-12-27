#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::system::error_code;
using namespace boost::posix_time;

class IoContext : public std::enable_shared_from_this<IoContext> {
    boost::asio::io_context io_context_;

    template <typename F>
    static void timer_handler(const error_code &e,
                              std::shared_ptr<deadline_timer> timer, F callback,
                              time_duration duration);

  public:
    auto &operator->() const { return io_context_; }

    auto &get() { return io_context_; }

    template <typename F>
    std::shared_ptr<deadline_timer> fixed_delay(const time_duration &duration,
                                                F callback);
};

template <typename F>
void IoContext::timer_handler(const error_code &e,
                              std::shared_ptr<deadline_timer> timer, F callback,
                              time_duration duration) {
    try {
        callback();
    } catch (const std::exception &e) {
        spdlog::warn("capture exception with timer handler {} (cancelled)",
                     e.what());
        return;
    }
    timer->expires_from_now(duration);
    timer->async_wait([=](const error_code &e) {
        timer_handler(e, timer, callback, duration);
    });
}

template <typename F>
std::shared_ptr<deadline_timer>
IoContext::fixed_delay(const time_duration &duration, F callback) {
    auto timer = std::make_shared<deadline_timer>(io_context_);
    timer->expires_from_now(duration);

    timer->async_wait([=](const error_code &e) {
        timer_handler(e, timer, callback, duration);
    });
    return timer;
}

class Acceptor {
    tcp::acceptor acceptor_;

  public:
    Acceptor(IoContext &context)
        : acceptor_(context.get(), tcp::endpoint(tcp::v4(), 8080)) {}
};

void start(IoContext &context) {
    auto acceptor = std::make_shared<Acceptor>(context);

    // set up a timer
    auto timer = context.fixed_delay(boost::posix_time::millisec(1000), []() {
        spdlog::info("timer thread={}",
                     std::hash<std::thread::id>{}(std::this_thread::get_id()));
        throw std::runtime_error("failed.");
    });
}

int main(int argc, char **argv) {
    auto io_context = std::make_shared<IoContext>();
    start(*io_context);

    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i != numThreads; ++i) {
        auto thread =
            std::thread([&io_context]() { io_context.get()->get().run(); });
        threads.push_back(std::move(thread));
    }
    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
            spdlog::info("thread {} joined", (intptr_t)thread.native_handle());
        }
    }
    spdlog::info("end!");
    return 0;
}
