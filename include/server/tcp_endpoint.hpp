#ifndef INCLUDED_tcp_endpoint_hpp
#define INCLUDED_tcp_endpoint_hpp

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace jm {

class IoContext;
class Acceptor;

using boost::asio::ip::tcp;

class TcpEndpoint : boost::noncopyable {
    tcp::socket socket_;

    void handleAccept(const boost::system::error_code &e);

  public:
    TcpEndpoint();

    void accept(Acceptor &acceptor);

    tcp::socket &socket() { return socket_; }
};
} // namespace jm

#endif // INCLUDED_tcp_endpoint_hpp