#include <net/channel_handler_context.hpp>
#include <net/http/http2_channel.hpp>
#include <net/http2_channel_handler.hpp>

using namespace jm::net::http;

Http2Channel::Http2Channel(u32 streamId, const ChannelHandlerContext &context)
    : streamId_(streamId), context_(context) {}

// void Http2Channel::dispatch(http::HeaderMap &&headers,
//                             Dispatcher const &dispatcher) {
//    auto request = Request{std::move(headers)};
//    auto response = dispatcher(request);
// }

// void Http2Channel::dispatch(jm::net::Http2ChannelHandler &handler,
//                             http::HeaderMap &&headers) {
//    // auto request = Request{std::move(headers)};
//    // auto response = handler.dispatch(request);
//    // handler.sendResponse(*this, request, response);
// }

HeaderMap Http2Channel::decodeHeaders(const std::span<u8> &data) {
   return hpackDecode_.decode(data);
}

std::vector<jm::u8> Http2Channel::encodeHeaders(const HeaderMap &headers) {
   return hpackEncode_.encode(headers);
}