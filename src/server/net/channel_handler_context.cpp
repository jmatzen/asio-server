#include <net/channel.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>

namespace jm::net {
void ChannelHandlerContext::writeFinal(const std::span<u8> &data) const {
   auto pipeline = pipeline_.lock();
   if (pipeline) {
      pipeline->write(data);
   }
}

} // namespace jm::net
