#include <net/channel.hpp>
#include <spdlog/spdlog.h>
#include <thread.hpp>

using namespace jm::net;

Channel::~Channel() { spdlog::info("channel {} closed", usize(this)); }

void Channel::startRead_(
    std::function<void(const std::span<u8> &)> const &callback) {
    auto buf = boost::asio::buffer(new uint8_t[1], 1);

    std::scoped_lock lock(mutex_);
    if (!socket_) {
        // socket has been destroyed, possibly on another thread
        return;
    }

    auto index = next_.fetch_add(1);

    socket_->async_read_some(buf, [=](const boost::system::error_code &error,
                                      std::size_t const bytes_transferred) {
        spdlog::info("{} {}", error.message(), bytes_transferred);
        if (bytes_transferred > 0) {
            const char *p = (char *)buf.data();

            std::string msg(p, bytes_transferred);

            {
                std::scoped_lock lock(mutex_);

                spdlog::info(
                    "{} thread={} index={} count={} heapSize={} waitingOn={} "
                    "lowestIndex={}",
                    std::is_heap(gatherHeap_.begin(), gatherHeap_.end()),
                    Thread::thisThreadId(), index, bytes_transferred,
                    gatherHeap_.size(), waitingOn_.load(),
                    gatherHeap_.empty() ? -1 : gatherHeap_.front().index);

                gatherHeap_.emplace_back(
                    Gather{index, {(u8 *)buf.data(), buf.size()}});
                std::push_heap(gatherHeap_.begin(), gatherHeap_.end());

                while (!gatherHeap_.empty() &&
                       gatherHeap_.front().index == waitingOn_) {

                    // spdlog::info("popping {}", gatherHeap_.front().index);
                    auto segment = gatherHeap_.front();
                    std::pop_heap(gatherHeap_.begin(), gatherHeap_.end());
                    gatherHeap_.pop_back();

                    waitingOn_++;

                    callback({segment.buf.first, segment.buf.second});

                    delete[] segment.buf.first;
                }
            }

            startRead(callback);

        } else {
            callback({(u8 *)nullptr, 0});
            std::scoped_lock lock(mutex_);
            socket_.reset();
        }
    });
}