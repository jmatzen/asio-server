#include <net/channel.hpp>
#include <spdlog/spdlog.h>
#include <thread.hpp>

using namespace jm;
using namespace jm::net;

// Initialize the static connection counter
std::atomic<u32> Channel::nextConnectionId_{1};

Channel::~Channel() { 
   spdlog::info("TCP connection {} closed", connectionId_); 
}

void Channel::write(const std::span<u8> &data) {
   if (socket_) {
      auto buf = boost::asio::buffer(new uint8_t[data.size()], data.size());
      memcpy(buf.data(), data.data(), data.size());
      spdlog::info("writing {} bytes", data.size());
      socket_->async_write_some(buf, [=](const boost::system::error_code& e, std::size_t const count) {
         spdlog::info("{} bytes written", count);
         delete[] (u8*)buf.data();
      });
   }
}

void Channel::startRead_(
    std::function<void(const std::span<u8> &)> const &callback) {
   auto buf = boost::asio::buffer(new uint8_t[1024], 1024);

   if (!socket_) {
      // socket has been destroyed, possibly on another thread
      return;
   }

   auto index = next_.fetch_add(1);

   socket_->async_read_some(buf, [=](const boost::system::error_code &error,
                                     std::size_t const bytes_transferred) {
      std::scoped_lock lock(mutex_);

      gatherHeap_.emplace_back(
          Gather{index, {(u8 *)buf.data(), bytes_transferred}});
      std::push_heap(gatherHeap_.begin(), gatherHeap_.end());

      // spdlog::info("{} thread={} index={} count={} heapSize={} waitingOn={} "
      //              "lowestIndex={}",
      //              std::is_heap(gatherHeap_.begin(), gatherHeap_.end()),
      //              Thread::thisThreadId(), index, bytes_transferred,
      //              gatherHeap_.size(), waitingOn_.load(),
      //              gatherHeap_.empty() ? -1 : gatherHeap_.front().index);

      while (!gatherHeap_.empty() && gatherHeap_.front().index == waitingOn_) {
         auto segment = gatherHeap_.front();

         std::pop_heap(gatherHeap_.begin(), gatherHeap_.end());
         gatherHeap_.pop_back();

         waitingOn_++;

         callback({segment.buf.first, segment.buf.second});

         delete[] segment.buf.first;
      }

      if (bytes_transferred > 0) {
         startRead_(callback);
      }
   });
}