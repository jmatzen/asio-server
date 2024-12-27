#ifndef INCLUDED_acceptor_hpp
#define INCLUDED_acceptor_hpp

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace jm {
using boost::asio::ip::tcp;

class IoContext;

class Acceptor : boost::noncopyable {
    tcp::acceptor acceptor_;

  public:
    Acceptor(IoContext &context, int port);
};
} // namespace jm
#endif // INCLUDED_acceptor_hpp