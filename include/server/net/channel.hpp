#ifndef INCLUDED__jm_net_channel_hpp
#define INCLUDED__jm_net_channel_hpp

#include <array>
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <types.hpp>

namespace jm::net {
static const int MAX_CONCURRENT_SCATTER_READS = 1;

struct Gather {
    u64 index;
    std::pair<u8 *, usize> buf;
};

inline bool operator<(const Gather &lhs, const Gather &rhs) {
    // this allows the heap to put the lowest value on top
    return lhs.index > rhs.index;
}

class Channel {
    using Socket = boost::asio::ip::tcp::socket;
    std::unique_ptr<Socket> socket_;
    std::mutex mutex_;
    std::atomic_uint64_t next_ = 0;
    std::atomic_uint64_t waitingOn_ = 0;
    std::vector<Gather> gatherHeap_;

    void startRead_(std::function<void(const std::span<u8>& )> const &callback);

  public:
    Channel(std::unique_ptr<Socket> &&socket) : socket_(std::move(socket)) {}

    ~Channel();

    void startRead(std::function<void(const std::span<u8>& )> const &callback) {
        for (int i = 0; i != MAX_CONCURRENT_SCATTER_READS; ++i) {
            startRead_(callback);
        }
    }

    void shutdown() {
        socket_->shutdown(boost::asio::socket_base::shutdown_both);
    }

    void close() {
        socket_->close();
    }

    void write(const std::span<u8>& data);
};
} // namespace jm::net

#endif // INCLUDED__jm_net_channel_hpp