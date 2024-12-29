#include "app_context.hpp"
#include <acceptor.hpp>
#include <io_context.hpp>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>

using namespace jm;
using boost::asio::ip::tcp;

Acceptor::Acceptor(
    int port,
    std::function<void(std::shared_ptr<net::Channel> const &)> const &fn)
    : acceptor_(AppContext::getContext().getIoContext().get(),
                tcp::endpoint(tcp::v4(), port)),
      handler_(fn) {
    spdlog::info("listening on port {}", port);
}

void Acceptor::accept() {
    auto endpoint = new TcpEndpoint{};
    endpoint->accept(*this);
}

void Acceptor::handleAccept(const boost::system::error_code &e,
                            boost::asio::ip::tcp::socket &s) {
    s.shutdown(boost::asio::socket_base::shutdown_both);
}

void Acceptor::handleConnection(std::unique_ptr<TcpEndpoint> tcpEndpoint,
                                const boost::system::error_code &e) {
    accept();
    handler_(nullptr);
    spdlog::info("handleConnection");
    auto endpoint = tcpEndpoint->socket().remote_endpoint();
    if (e) {
        spdlog::warn("connect failed: {}", e.what());
    } else {
        spdlog::info("connection from {}:{}", endpoint.address().to_string(),
                     endpoint.port());
    }
}
