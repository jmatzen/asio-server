#include <acceptor.hpp>
#include <app_context.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>
#include <thread.hpp>
#include <thread>
#include <vector>

using namespace jm;

class MainClass : public Component {
    std::shared_ptr<boost::asio::deadline_timer> timer;
    std::unique_ptr<Acceptor> acceptor_;

  public:
    void postConstruct() override {

        acceptor_ = std::make_unique<Acceptor>(8080, [](auto &&channel) {
            auto channel_ = channel.release();
            auto &io = AppContext::getContext().getIoContext().get();
            boost::asio::ip::tcp::socket &s = channel_->socket();

            auto buf = boost::asio::buffer(new uint8_t[1024], 1024);
            s.async_read_some(buf, [=](const boost::system::error_code &error,
                                       std::size_t bytes_transferred) {
                spdlog::info("read {} bytes", bytes_transferred);
                const char *p = (char *)buf.data();
                std::string s(p, bytes_transferred);
                spdlog::info("{}", s);

                delete[] (uint8_t *)buf.data();
                delete channel_;
            });
        });
        acceptor_->accept();

        spdlog::info("MainClass::postConstruct");
        timer = AppContext::getContext().getIoContext().fixedDelay(
            boost::posix_time::millisec(1000), []() {
                spdlog::info("timer thread={}", Thread::thisThreadId());
                // throw std::runtime_error("failed.");
            });
    }
};

int main(int argc, char **argv) {

    AppContext::run<MainClass>();

    return 0;
}
