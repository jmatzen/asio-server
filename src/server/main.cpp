#include <acceptor.hpp>
#include <app_context.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread.hpp>
#include <thread>
#include <vector>

using namespace jm;

class MainClass : public Component {
    std::shared_ptr<boost::asio::deadline_timer> timer;
    std::unique_ptr<Acceptor> acceptor_;

  public:

    void postConstruct() override {

        acceptor_ = std::make_unique<Acceptor>(8080);
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
