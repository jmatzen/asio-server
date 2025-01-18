#include <net/channel_pipeline.hpp>
#include <spdlog/spdlog.h>

using namespace jm::net;

ChannelPipeline::~ChannelPipeline()
{
   spdlog::info("~ChannelPipeline()");
}

void ChannelPipeline::addLast(std::unique_ptr<ChannelHandler> &&uniqHandler,
                              const std::string_view &name)
{
   checkDuplicateName(name);
   auto handler = std::shared_ptr<ChannelHandler>(uniqHandler.release());
   auto context =
       std::make_shared<ChannelHandlerContext>(shared_from_this(), handler);
   if (!handlers_.empty())
   {
      context->setPrev(handlers_.back().second);
      handlers_.back().second->setNext(context);
   }
   handlers_.push_back(std::make_pair(std::string(name), context));
}

void ChannelPipeline::checkDuplicateName(const std::string_view &name) const
{
   for (auto &handler : handlers_)
   {
      if (handler.first == name)
      {
         throw std::runtime_error("duplicate channel handler name");
      }
   }
}

void ChannelPipeline::startRead()
{
   channel_->startRead([=](const std::span<u8> &buf) {
      if (buf.size() == 0)
      {
         onClose_();
      }
      else if (!handlers_.empty())
      {
         try
         {
            handlers_.front().second->onChannelRead(buf);
         }
         catch (const std::exception &e)
         {
            spdlog::warn("an exception occured: {}", e.what());
            channel_->close();
         }
      }
   });
}

void ChannelPipeline::write(const std::span<u8>& data) {
   channel_->write(data);
}
