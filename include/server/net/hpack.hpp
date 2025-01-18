#ifndef included__jm_net_hpack_hpp
#define included__jm_net_hpack_hpp

#include <memory>
#include <span>
#include <string>
#include <vector>
#include <types.hpp>
#include <unordered_map>

struct hpack_table;

namespace jm::net {

class HPack {
   hpack_table * const hpackTable_{};

 public:
   HPack();
   ~HPack();

   std::unordered_map<std::string, std::string>
   decode(std::span<u8> const &span) const;

   std::vector<u8> encode(const std::unordered_map<std::string, std::string>& headers);
};

} // namespace jm::net
#endif // included__jm_net_hpack_hpp