#include <acceptor.hpp>
#include <app_context.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <net/channel.hpp>
#include <net/channel_handler.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>
#include <thread.hpp>
#include <thread>
#include <vector>
#include <set>

using namespace jm;

class MyChannelHandler : public net::ChannelHandler {
    void onChannelRead(const std::span<u8> &buf) override {
        std::string s((char *)buf.data(), buf.size());
        spdlog::info("===> {}", s);
    }
};

class MainClass : public Component {
    std::shared_ptr<boost::asio::deadline_timer> timer;
    std::unique_ptr<Acceptor> acceptor_;
    std::set<std::shared_ptr<net::ChannelPipeline>> pipelines_;
    std::mutex pipelinesMutex_;

  public:
    void postConstruct() override {

        acceptor_ = std::make_unique<Acceptor>(8080, [=](auto &&s) {
            auto channel = std::make_unique<net::Channel>(std::move(s));

            auto pipeline =
                std::make_shared<net::ChannelPipeline>(std::move(channel));
            pipeline->addLast(std::make_unique<MyChannelHandler>(), "0");
            pipeline->onClose([=](){
                spdlog::info("closing!");
                std::scoped_lock lk(pipelinesMutex_);
                pipelines_.erase(pipeline);
            });
            pipeline->startRead();
            pipelines_.insert(pipeline);
            spdlog::info("created pipeline");
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
