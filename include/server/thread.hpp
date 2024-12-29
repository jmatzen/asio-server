#ifndef INCLUDED_thread_hpp
#define INCLUDED_thread_hpp

#include <boost/noncopyable.hpp>
#include <thread>

namespace jm {
class Thread : boost::noncopyable {
    int thread_id_ = nextThreadId();
    std::thread thread_;

    static void setLocalThreadId(int id);

    static int nextThreadId();

    template<typename F>
    auto fnWrapper(const F& f) {
        int id = thread_id_;
        return [=]() {
            setLocalThreadId(id);
            f();
        };
    }

    // void operator=(Thread&&) = delete;

  public:
    Thread(Thread &&rhs)
        : thread_(std::move(rhs.thread_)), thread_id_(rhs.thread_id_) {}

    template <typename F>
    Thread(const F &func)
        : thread_(fnWrapper(func)) {}

    Thread& operator=(Thread&& rhs) {
        std::swap(thread_, rhs.thread_);
        std::swap(thread_id_, rhs.thread_id_);
        return *this;
    }

    std::thread *operator->() { return &thread_; }

    int id() const { return thread_id_; }

    static int thisThreadId();
};
} // namespace jm

#endif // INCLUDED_thread_hpp