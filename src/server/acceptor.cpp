#include <acceptor.hpp>
#include <io_context.hpp>
#include <tcp_endpoint.hpp>
#include <spdlog/spdlog.h>
#include "app_context.hpp"

using namespace jm;
using boost::asio::ip::tcp;

Acceptor::Acceptor(int port)
    : 
      acceptor_(AppContext::getContext().getIoContext().get(), tcp::endpoint(tcp::v4(), port)) {
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