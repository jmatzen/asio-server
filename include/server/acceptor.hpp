#ifndef INCLUDED_acceptor_hpp
#define INCLUDED_acceptor_hpp

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace jm {
using boost::asio::ip::tcp;

class IoContext;

class Acceptor : boost::noncopyable {
    tcp::acceptor acceptor_;

    void handleAccept(const boost::system::error_code &e,
                      boost::asio::ip::tcp::socket& s);

  public:
    Acceptor(int port);

    void accept();

    tcp::acceptor* operator->() {
        return &acceptor_;
    }
};
} // namespace jm
#endif // INCLUDED_acceptor_hpp