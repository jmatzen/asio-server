#ifndef included__jm_net_http_request_hpp
#define included__jm_net_http_request_hpp

#include "headers.hpp"
#include <memory>

namespace jm::net::http {
class Request {
 public:
   Request(HeaderMap &&headers) : headers_(std::move(headers)) {}

 private:
   HeaderMap headers_;
};
} // namespace jm::net::http

#endif // included__jm_net_http_request_hpp
