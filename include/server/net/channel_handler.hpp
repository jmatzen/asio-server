#ifndef included__jm_net_channel_handler_hpp
#define included__jm_net_channel_handler_hpp

#include "../types.hpp"
#include <span>

namespace jm::net {
class ChannelHandler {
  public:
    virtual ~ChannelHandler() = default;
    virtual void write(const std::span<u8> &buf) {}
    virtual void onChannelRead(const std::span<u8> &buf) {}

};

template <typename T>
concept IsChannelHandler = std::is_base_of_v<ChannelHandler, T>;

} // namespace jm::net

#endif // included__jm_net_channel_handler_hpp