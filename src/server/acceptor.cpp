#include "app_context.hpp"
#include <acceptor.hpp>
#include <io_context.hpp>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>

using namespace jm;
using boost::asio::ip::tcp;

Acceptor::Acceptor(
    int port, std::function<void(std::unique_ptr<tcp::socket> &&)> const &fn)
    : acceptor_(AppContext::getContext().getIoContext().get(),
                tcp::endpoint(tcp::v4(), port)),
      handler_(fn) {
    spdlog::info("listening on port {}", port);
}

void Acceptor::accept() {
    // auto endpoint = new TcpEndpoint{};
    // endpoint->accept(*this);
    tcp::socket* socket = new tcp::socket(AppContext::getContext().getIoContext().get());
    acceptor_.async_accept(
        *socket, [this,socket](const boost::system::error_code &e) {
            this->handleConnection(std::unique_ptr<tcp::socket>(socket), e);
        });
}

void Acceptor::handleAccept(const boost::system::error_code &e,
                            boost::asio::ip::tcp::socket &s) {
    s.shutdown(boost::asio::socket_base::shutdown_both);
}

void Acceptor::handleConnection(std::unique_ptr<boost::asio::ip::tcp::socket>&& socket,
                                const boost::system::error_code &e) {
    // accept a new connection
    accept();

    if (e) {
        spdlog::warn("connect failed: {}", e.what());
        // tcpEndpoint should be destroyed at the end of this call because it's
        // not passed to the handler.
    } else {
        auto endpoint = socket->remote_endpoint();
        spdlog::info("connection from {}:{}", endpoint.address().to_string(),
                     endpoint.port());
        handler_(std::move(socket));
    }
}
