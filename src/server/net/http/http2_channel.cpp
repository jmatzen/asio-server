#include <net/http/http2_channel.hpp>

using namespace jm::net::http;

Http2Channel::Http2Channel(HeaderMap &&headers)
    : headers_(std::move(headers)) {}
