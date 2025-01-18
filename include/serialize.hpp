#ifndef included__jm__serialize_hpp
#define included__jm__serialize_hpp

#include "server/types.hpp"
#include <iterator>

namespace jm::ser {

template <typename T>
concept IsInteger =
    std::is_integral_v<T>;

template<typename T>
concept IsEnum  = std::is_enum_v<T>;

template <typename Container>
concept BackInsertableContainer =
    requires(Container c, typename Container::value_type v) {
       std::back_inserter(c); // Supports back insertion
       c.push_back(v);        // Has push_back method
       requires std::same_as<typename Container::value_type, uint8_t>;
    };

template <int N, IsInteger T, BackInsertableContainer Container>
void serialize(Container &container, T value) {
   static_assert(N > 0, "N must be positive.");
   for (int i = 0; i != N; ++i) {
      int shift = (N-i-1)*8;
      int byte = (value>>shift)&0xff;
      container.push_back(byte);
   }
}

template <int N, IsEnum T, BackInsertableContainer Container>
void serialize(Container &container, T value) {
   serialize<N>(container, static_cast<u32>(value));
}

} // namespace jm::ser
#endif // included__jm__serialize_hpp
