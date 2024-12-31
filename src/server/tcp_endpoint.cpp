#include <acceptor.hpp>
#include <app_context.hpp>
#include <io_context.hpp>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>

using namespace jm;
using boost::asio::ip::tcp;

void handleAccept(const boost::system::error_code &e) {}

TcpEndpoint::TcpEndpoint()
    : socket_(AppContext::getContext().getIoContext().get()) {}

void TcpEndpoint::accept(Acceptor &acceptor) {
    // spdlog::info("waiting for connection");
    // acceptor->async_accept(
    //     socket_, [this, &acceptor](const boost::system::error_code &e) {
    //         acceptor.handleConnection(std::unique_ptr<TcpEndpoint>(this), e);
    //     });
}

void TcpEndpoint::handleAccept(const boost::system::error_code& e) {
    if (e) {
        spdlog::warn("connect failed: {}", e.what());
    } else {
        /// do something
    }
}