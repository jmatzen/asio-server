#include <acceptor.hpp>
#include <app_context.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <net/channel.hpp>
#include <net/channel_handler.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>
#include <thread.hpp>
#include <thread>
#include <vector>

using namespace jm;

class MyChannelHandler : public net::ChannelHandler {
    void onChannelRead(const net::ChannelHandlerContext  &ctx, const std::span<u8> &buf) override {
        std::string s((char *)buf.data(), buf.size());
        for (int offset = 0; offset < buf.size(); offset += 16) {
            std::string line;
            line += fmt::format("{:04x} ", offset);
            for (int q = offset; q < offset + 16; ++q) {
                if (q < buf.size()) {
                    line += fmt::format("{:02x} ", u8(s[q]));
                } else {
                    line += "   ";
                }
            }
            line += " ";
            for (int q = offset; q < offset + 16 && q < buf.size(); ++q) {
                if (q < buf.size()) {
                    line += s[q] >= ' ' && s[q] <= 127 ? s[q] : '.';
                }
            }

            spdlog::info(line);
        }
        ctx.next(buf);
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
            std::weak_ptr<net::ChannelPipeline> weakPipeline = pipeline;
            pipeline->onClose([=]() {
                spdlog::info("closing!");
                auto pipeline = weakPipeline.lock();
                if (pipeline) {
                    std::scoped_lock lk(pipelinesMutex_);
                    pipelines_.erase(pipeline);
                }
            });
            pipelines_.insert(pipeline);
            pipeline->startRead();
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
