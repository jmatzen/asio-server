#ifndef included__jm_net_deserialize_hpp
#define included__jm_net_deserialize_hpp

#include "../types.hpp"
#include <arpa/inet.h>
#include <concepts>
#include <type_traits>

namespace jm::net
{
inline u32 ntoh_(u32 value)
{
   return ntohl(value);
}

template <typename T>
concept IsNtohConvertable = std::is_integral_v<T>;

template <typename T>
concept HasPointerAndIncrementable = requires(T it) {
   { *it };                     // Can be dereferenced
   { it++ } -> std::same_as<T>; // Can be incremented
   { it.base() };               // supports base()
};

template <IsNtohConvertable T, HasPointerAndIncrementable U>
inline T deserialize(U &it)
{
   T value;
   memcpy(&value, it.base(), sizeof(T));
   it += sizeof(T);
   value = ntoh_(value);
   return value;
}

template <IsNtohConvertable T> inline T deserialize(const u8 *p)
{
   T value;
   memcpy(&value, p, sizeof(T));
   value = ntoh_(value);
   return value;
}

} // namespace jm::net

#endif // included__jm_net_deserialize_hpp