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

HPack::HPack() : hpackTable_(hpack_table_new(0)) {}

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

std::vector<jm::u8>
HPack::encode(std::unordered_map<std::string, std::string> const &headers) {
   hpack_headerblock *hblk = hpack_headerblock_new();

   for (auto &item : headers) {
      hpack_header_add(hblk, item.first.c_str(), item.second.c_str(),
                       HPACK_INDEX);
   }

   size_t encodedLen{};
   auto dataPtr = hpack_encode(hblk, &encodedLen, hpackTable_);
   auto vec = std::vector<u8>(dataPtr, dataPtr + encodedLen);

   hpack_headerblock_free(hblk);

   return vec;
}

// hpack helpers
extern "C" void freezero(void *p, size_t len) {
   memset(p, 0, len);
   free(p);
}

extern "C" void *recallocarray(void *p, size_t o, size_t n, size_t size) {
   return realloc(p, n * size);
}
