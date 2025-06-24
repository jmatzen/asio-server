#ifndef included__jm_net_http2_channel_handler_hpp
#define included__jm_net_http2_channel_handler_hpp

#include "channel_handler.hpp"
#include "http/headers.hpp"
#include "http/http2_channel.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include <unordered_map>
#include <vector>

namespace jm::net::http {
class Http2Channel;
class Request;
class Response;

} // namespace jm::net::http

namespace jm::net {

class Http2ChannelHandler : public net::ChannelHandler {
   using Dispatcher = std::function<http::Response(const http::Request &)>;

 public:
   Http2ChannelHandler(Dispatcher dispatcher) : dispatcher_(dispatcher) {}

 private:
   std::mutex mutex_;
   Dispatcher dispatcher_;

   // Flow control state
   u32 connectionWindowSize_ = 65535;  // Initial connection window size
   u32 initialStreamWindowSize_ = 65535;  // Initial stream window size
   std::unordered_map<u32, u32> streamWindowSizes_;  // Per-stream window sizes

   struct Frame;

   enum class State {
      FAILED,
      CONNECTION,
      READING,
   };
   std::vector<u8> buffer_;
   State state_ = State::CONNECTION;
   std::unordered_map<u32, std::unique_ptr<http::Http2Channel>> channels_;

   void onChannelRead(const net::ChannelHandlerContext &ctx,
                      const std::span<u8> &buf)  override;

   void onChannelReadConnectionState(const net::ChannelHandlerContext &ctx,
                                     const std::span<u8> &buf);

   void onChannelReadReadingState(const net::ChannelHandlerContext &ctx,
                                  const std::span<u8> &buf);

   void processFrames(const net::ChannelHandlerContext &ctx);

   std::vector<u8>::iterator processNextFrame(const net::ChannelHandlerContext &ctx,
                                              std::vector<u8>::iterator begin);

   void processFrameSettings(const Frame &frame);

   void processFrameWindowUpdate(const Frame &frame);

   void processFrameHeaders(const Frame &frame);

   void processFrameData(const Frame &frame);

   void encodeHeaderFrame(const ChannelHandlerContext &context,
                          const http::HeaderMap &headerMap, u32 streamId);

   void sendInitialSettingsFrame(const ChannelHandlerContext& context);

   // Flow control methods
   void updateConnectionWindow(i32 delta);
   void updateStreamWindow(u32 streamId, i32 delta);
   void sendWindowUpdate(const ChannelHandlerContext& context, u32 streamId, u32 increment);
   bool canSendData(u32 streamId, u32 dataSize);
   void consumeConnectionWindow(u32 dataSize);
   void consumeStreamWindow(u32 streamId, u32 dataSize);


 public:
   void sendResponse(const ChannelHandlerContext &context,
                     const http::Http2Channel &channel,
                     const http::Request &request,
                     const http::Response &response);

   // http::Response dispatch(const http::Request& request);
};
} // namespace jm::net
#endif // included__jm_net_http2_channel_handler_hpp