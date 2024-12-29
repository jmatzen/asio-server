#ifndef INCLUDED_io_context_hpp
#define INCLUDED_io_context_hpp

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <memory>

namespace jm {
using boost::asio::deadline_timer;
using boost::system::error_code;
using namespace boost::posix_time;

class IoContext : boost::noncopyable {
    boost::asio::io_context io_context_;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard;

    template <typename F>
    static void timerHandler(const boost::system::error_code &e,
                             const std::shared_ptr<deadline_timer> &timer,
                             const F &callback, time_duration duration);

    static void handleException(const std::exception &e);

  public:
    IoContext() : work_guard(io_context_.get_executor()) {}
    auto &operator->() const { return io_context_; }

    auto &get() { return io_context_; }

    template <typename F>
    std::shared_ptr<deadline_timer> fixedDelay(const time_duration &duration,
                                               const F &callback);

    template <typename F> void post(const F &callback) {
        io_context_.post(callback);
    }
};

template <typename F>
void IoContext::timerHandler(const error_code &e,
                             const std::shared_ptr<deadline_timer> &timer,
                             const F &callback, time_duration duration) {
    try {
        callback();
    } catch (const std::exception &e) {
        handleException(e);
        return;
    }
    timer->expires_from_now(duration);
    timer->async_wait([=](const error_code &e) {
        timerHandler(e, timer, callback, duration);
    });
}

template <typename F>
std::shared_ptr<deadline_timer>
IoContext::fixedDelay(const time_duration &duration, const F &callback) {
    auto timer = std::make_shared<deadline_timer>(io_context_);

    timer->expires_from_now(duration);

    timer->async_wait([=](const error_code &e) {
        timerHandler(e, timer, callback, duration);
    });

    return timer;
}

} // namespace jm

#endif // INCLUDED_io_context_hpp
