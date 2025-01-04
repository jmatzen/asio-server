#ifndef included__jm_net_deserialize_hpp
#define included__jm_net_deserialize_hpp

#include "../types.hpp"
#include <arpa/inet.h>
#include <concepts>
#include <span>
#include <type_traits>

namespace jm::net
{

/**
 * @brief Converts a 32-bit integer from network byte order to host byte order.
 *
 * This function acts as a wrapper around the POSIX `ntohl` function to
 * handle endianness conversion for 32-bit integers received from the network.
 *
 * @param value The 32-bit integer in network byte order.
 * @return The 32-bit integer in host byte order.
 */
inline u32 ntoh_(u32 value)
{
   return ntohl(value);
}

/**
 * @brief Concept to determine if a type is convertible through network byte
 * order functions.
 *
 * This concept checks whether a type `T` is integral, thus suitable for
 * network-to-host order conversion functions.
 */
template <typename T>
concept IsNtohConvertable = std::is_integral_v<T>;

/**
 * @brief Concept to verify if an iterator is dereferenceable, incrementable,
 * and has a `base()` method.
 *
 * This concept validates that the iterator type `T` can be dereferenced,
 * supports post-increment operations, and possesses a `base()` function. It's
 * typically used for iterators such as those involving reverse iteration.
 */
template <typename T>
concept HasPointerAndIncrementable = requires(T it) {
   { *it };                     // Can be dereferenced
   { it++ } -> std::same_as<T>; // Can be incremented
   { it.base() };               // supports base()
};

/**
 * @brief Deserializes a value of type `T` from a given iterator.
 *
 * This function reads a value from the specified iterator object by copying
 * bytes from the iterator's current position (given using its `base()` method),
 * then advances the iterator by the size of `T` bytes. The extracted value is
 * converted from network byte order to host byte order.
 *
 * @tparam T The type of the value to be deserialized.
 * @tparam U The type of the iterator, constrained by
 * `HasPointerAndIncrementable`.
 * @param it The iterator from which the value will be deserialized.
 * @return The deserialized and byte-order converted value of type `T`.
 */
template <IsNtohConvertable T, HasPointerAndIncrementable U>
inline T deserialize(U &it)
{
   T value{};
   for (int i = 0; i != sizeof(U); ++i)
   {
      value = (value << 8) | *it++;
   }
   return value;
}

/**
 * @brief Deserializes a value of type `T` from a pointer to unsigned 8-bit
 * integers.
 *
 * This function copies a value of type `T` from the given byte pointer `p`
 * and converts the value from network byte order to host byte order.
 *
 * @tparam T The type of the value to be deserialized.
 * @param p A pointer to the bytes from which the value is deserialized.
 * @return The deserialized and byte-order converted value of type `T`.
 */
template <IsNtohConvertable T> inline T deserialize(const u8 *p)
{
   T value;
   memcpy(&value, p, sizeof(T));
   value = ntoh_(value);
   return value;
}

/**
 * @brief Deserializes a value of type `T` built from a byte sequence.
 *
 * This overload reads a sequence of bytes, specified by `count`, and assembles
 * them into a value of type `T`, assuming big-endian format (network byte
 * order). This function is useful when the number of bytes is not equal to the
 * size of `T`.
 *
 * @tparam T The type of the value to be deserialized.
 * @param p A pointer to the sequence of bytes.
 * @param count The number of bytes to use in the deserialization.
 * @return The value of type `T` constructed from the byte sequence.
 */
template <typename T, int N> T deserialize(const u8 *p)
{
   T value{};
   for (int i = 0; i != N; ++i)
   {
      value = (value << 8) | p[i];
   }
   return value;
}

/**
 * @brief Deserializes a value of type `T` from a span of bytes.
 *
 * This function processes a span of bytes, `bits`, to reconstruct a value of
 * type `T`, interpreting the bytes in big-endian order (typical network byte
 * order). Each byte contributes to forming the resultant value by shifting it
 * left and integrating the current byte.
 *
 * @tparam T The type of the value to be deserialized.
 * @param bits A span of bytes representing the serialized data.
 * @return The deserialized value of type `T`.
 */
template <typename T> T deserialize(const std::span<u8> &bits)
{
   T value{};
   for (u8 byte : bits)
   {
      value = (value << 8) | byte;
   }
   return value;
}

} // namespace jm::net

#endif // included__jm_net_deserialize_hpp
