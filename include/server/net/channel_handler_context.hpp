#ifndef included__jm_net_channel_handler_context_hpp
#define included__jm_net_channel_handler_context_hpp

#include <memory>
#include "channel.hpp"
#include "channel_handler.hpp"

namespace jm::net {
class ChannelPipeline;
class ChannelHandler;
class Channel;
class ChannelHandlerContext {
    std::weak_ptr<ChannelPipeline> pipeline_;
    std::shared_ptr<ChannelHandler> handler_;
    std::shared_ptr<ChannelHandlerContext> next_;
    std::shared_ptr<ChannelHandlerContext> prev_;

  public:
    ChannelHandlerContext(const std::shared_ptr<ChannelPipeline> &pipeline,
                          const std::shared_ptr<ChannelHandler> &handler)
        : pipeline_(pipeline), handler_(handler) {}

    void setNext(const std::shared_ptr<ChannelHandlerContext> &next) {
        next_ = next;
    }

    void setPrev(const std::shared_ptr<ChannelHandlerContext> &prev) {
        prev_ = prev;
    }

    auto getNext() const {
        return next_;
    }

    auto getPrev() const {
        return prev_;
    }

    void onChannelRead(const std::span<u8>& buf) {
        handler_->onChannelRead(buf);
        if (next_) {
            next_->onChannelRead(buf);
        }
    }
    
};
} // namespace jm::net
#endif // included__jm_net_channel_handler_context_hpp