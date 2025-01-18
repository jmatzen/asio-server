#ifndef included__jm_net_channel_pipeline_hpp
#define included__jm_net_channel_pipeline_hpp

#include "../types.hpp"
#include "channel.hpp"
#include "channel_handler.hpp"
#include "channel_handler_context.hpp"
#include <expected>
#include <tuple>
#include <vector>

namespace jm::net {
class ChannelPipeline : public std::enable_shared_from_this<ChannelPipeline> {
    std::vector<std::pair<std::string, std::shared_ptr<ChannelHandlerContext>>>
        handlers_;
    std::unique_ptr<Channel> channel_;
    std::function<void()> onClose_;

    void checkDuplicateName(const std::string_view &name) const;

  public:
    ChannelPipeline(std::unique_ptr<Channel> &&channel)
        : channel_(std::move(channel)) {}

    ~ChannelPipeline();

    void onChannelRead(const std::span<u8> &buf) const {
        for (auto &handler : handlers_) {
            // handler.second->onChannelRead(buf);
        }
    }

    void addLast(std::unique_ptr<ChannelHandler> &&handler,
                 const std::string_view &name);

    void startRead();

    void onClose(const std::function<void()>& callback) {
        onClose_ = callback;
    }

    Channel& getChannel() const {
        return *channel_.get();
    }

    void write(const std::span<u8>& data);
};

} // namespace jm::net
#endif // included__jm_net_channel_pipeline_hpp