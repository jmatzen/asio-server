#ifndef included__jm_net_http_http2_channel
#define included__jm_net_http_http2_channel

#include <string>
#include <types.hpp>
#include <unordered_map>

namespace jm::net::http {
using HeaderMap = std::unordered_map<std::string, std::string>;
class Http2Channel {
   HeaderMap headers_;

 public:
   Http2Channel(HeaderMap &&headers);
};
} // namespace jm::net::http

#endif // included__jm_net_http_http2_channel