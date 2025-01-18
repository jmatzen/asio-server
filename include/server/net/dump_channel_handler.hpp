#ifndef included__jm_net_dump_channel_handler_hpp
#define included__jm_net_dump_channel_handler_hpp

#include "channel_handler.hpp"

namespace jm::net {

class DumpChannelHandler : public net::ChannelHandler {
   void onChannelRead(const net::ChannelHandlerContext &ctx,
                      const std::span<u8> &buf) override;

   void write(const ChannelHandlerContext &ctx,
                      const std::span<u8> &buf) const override;
};
} // namespace jm::net
#endif // included__jm_net_dump_channel_handler_hpp