#ifndef included__jm_net_http__headers_hpp
#define included__jm_net_http__headers_hpp

#include <unordered_map>
#include <string>

namespace jm::net::http {
   using HeaderMap = std::unordered_map<std::string, std::string>;

}

#endif // included__jm_net_http__headers_hpp
