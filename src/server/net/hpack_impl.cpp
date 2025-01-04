#include <net/hpack.hpp>

extern "C" {
#include <hpack.h>
}

namespace {
const int hpackInit = hpack_init();
} // namespace

using namespace jm::net;

class HPackImpl {
 public:
};

HPack::HPack() { hpackTable_ = hpack_table_new(0); }

HPack::~HPack() {
   if (hpackTable_) {
      hpack_table_free(hpackTable_);
   }
}

std::unordered_map<std::string, std::string>
HPack::decode(std::span<u8> const &buffer) const {
   std::unordered_map<std::string, std::string> headers;
   auto hpackHeaderBlock =
       hpack_decode(buffer.data(), buffer.size(), hpackTable_);

   for (auto item = hpackHeaderBlock->tqh_first;
        item != *hpackHeaderBlock->tqh_last; item = item->hdr_entry.tqe_next) {
      headers.emplace(item->hdr_name, item->hdr_value);
   }
   return headers;
}