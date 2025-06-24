#include <acceptor.hpp>
#include <app_context.hpp>
#include <chrono>
#include <io_context.hpp>
#include <memory>
#include <net/channel.hpp>
#include <net/channel_handler.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>
#include <net/dump_channel_handler.hpp>
#include <net/http2_channel_handler.hpp>
#include <set>
#include <spdlog/spdlog.h>
#include <tcp_endpoint.hpp>
#include <thread.hpp>
#include <thread>
#include <vector>

#include <reactive/once.hpp>

using namespace jm;

class MainClass : public Component {
   std::shared_ptr<boost::asio::deadline_timer> timer;
   std::unique_ptr<Acceptor> acceptor_;
   std::set<std::shared_ptr<net::ChannelPipeline>> pipelines_;
   std::mutex pipelinesMutex_;

 public:
   void postConstruct() override {

      auto dispatcher = [](const net::http::Request &) -> net::http::Response {
         return net::http::Response::builder()
             .headers(net::http::HeaderMap{
                 {"server", "rapidray/1"},
                 {"content-length", "0"},
             })
             .statusCode(net::http::StatusCode::NotFound)
             .build();
      };

      acceptor_ = std::make_unique<Acceptor>(8080, [=](auto &&s) {
         auto channel = std::make_unique<net::Channel>(std::move(s));
         u32 connectionId = channel->getConnectionId();

         auto pipeline =
             std::make_shared<net::ChannelPipeline>(std::move(channel));
         pipeline->addLast(std::make_unique<net::DumpChannelHandler>(), "dump");
         pipeline->addLast(
             std::make_unique<net::Http2ChannelHandler>(dispatcher), "http2");

         std::weak_ptr<net::ChannelPipeline> weakPipeline = pipeline;
         pipeline->onClose([=]() {
            spdlog::info("TCP connection {} pipeline closing!", connectionId);
            auto pipeline = weakPipeline.lock();
            if (pipeline) {
               std::scoped_lock lk(pipelinesMutex_);
               pipelines_.erase(pipeline);
            }
         });
         pipelines_.insert(pipeline);
         pipeline->startRead();
         spdlog::info("created pipeline for TCP connection {}", connectionId);
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
