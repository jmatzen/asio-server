#include <net/channel_handler.hpp>
#include <net/channel_handler_context.hpp>

namespace jm::net {
void ChannelHandler::write(const ChannelHandlerContext &ctx,
                           const std::span<u8> &data) const {
   ctx.writeNext(data);
}
} // namespace jm::net