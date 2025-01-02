#ifndef included__jm_net_http2_channel_handler_hpp
#define included__jm_net_http2_channel_handler_hpp

#include "channel_handler.hpp"
#include <vector>

namespace jm::net
{

class Http2ChannelHandler : public net::ChannelHandler
{
   std::mutex mutex_;
   
   struct Frame;

   enum class State
   {
      FAILED,
      CONNECTION,
      READING,
   };
   std::vector<u8> buffer_;
   State state_ = State::CONNECTION;

   void onChannelRead(const net::ChannelHandlerContext &ctx,
                      const std::span<u8> &buf) override;

   void onChannelReadConnectionState(const net::ChannelHandlerContext &ctx,
                                     const std::span<u8> &buf);

   void onChannelReadReadingState(const net::ChannelHandlerContext &ctx,
                                  const std::span<u8> &buf);

   void processFrames();
   std::vector<u8>::iterator processNextFrame(std::vector<u8>::iterator begin);
   void processFrameSettings(const Frame &frame);
   void processFrameWindowUpdate(const Frame &frame);
   void processFrameHeaders(const Frame &frame);
};
} // namespace jm::net
#endif // included__jm_net_http2_channel_handler_hpp