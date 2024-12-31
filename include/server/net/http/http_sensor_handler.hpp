#ifndef included__http_sensor_handler_hpp
#define included__http_sensor_handler_hpp

#include <net/channel_handler.hpp>

namespace jm::net::http {
class HttpSensorChannelHandler : public ChannelHandler {
    void write(const std::span<u8> &buf) override {}
    void onReadChannel(const std::span<u8> &buf) override {}
};
} // namespace jm::net::http

#endif // included__http_sensor_handler_hpp
