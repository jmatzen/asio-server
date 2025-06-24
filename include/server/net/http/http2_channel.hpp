#ifndef included__jm_net_http_http2_channel
#define included__jm_net_http_http2_channel

#include "headers.hpp"
#include "request.hpp"
#include "response.hpp"
#include <net/hpack.hpp>

#include <functional>
#include <string>
#include <types.hpp>
#include <unordered_map>

namespace jm::net {

class ChannelHandlerContext;
class Http2ChannelHandler;

namespace http {
class Http2Channel {
   using Dispatcher = std::function<http::Response(const http::Request &)>;

   u32 streamId_;
   const ChannelHandlerContext &context_;
   HPack hpackEncode_;
   HPack hpackDecode_;

 public:
   Http2Channel(u32 frameId, const ChannelHandlerContext &context);

   // void dispatch(http::HeaderMap&& headers, Dispatcher const &dispatcher);

   // void dispatch(Http2ChannelHandler& handler, http::HeaderMap&& headers);

   http::HeaderMap decodeHeaders(const std::span<u8> &data);

   std::vector<u8> encodeHeaders(const http::HeaderMap &headers);

   u32 getStreamId() const { return streamId_; }
};
} // namespace http
} // namespace jm::net

#endif // included__jm_net_http_http2_channel