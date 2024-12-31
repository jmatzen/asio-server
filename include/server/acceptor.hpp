#ifndef INCLUDED_acceptor_hpp
#define INCLUDED_acceptor_hpp

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace jm {
using boost::asio::ip::tcp;

class IoContext;
class TcpEndpoint;

namespace net {
class Channel;
}

class Acceptor : boost::noncopyable {
    tcp::acceptor acceptor_;
    std::function<void(std::unique_ptr<boost::asio::ip::tcp::socket>&&)> handler_;

    void handleAccept(const boost::system::error_code &e,
                      boost::asio::ip::tcp::socket &s);

  public:
    Acceptor(
        int port,
        std::function<void(std::unique_ptr<boost::asio::ip::tcp::socket>&&)> const &fn);

    void accept();

    void handleConnection(std::unique_ptr<boost::asio::ip::tcp::socket> &&socket, const boost::system::error_code &e);

    tcp::acceptor *operator->() { return &acceptor_; }
};
} // namespace jm
#endif // INCLUDED_acceptor_hpp