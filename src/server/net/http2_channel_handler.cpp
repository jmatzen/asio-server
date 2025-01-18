#include <net/channel.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>
#include <net/deserialize.hpp>
#include <net/hpack.hpp>
#include <net/http2_channel_handler.hpp>
#include <serialize.hpp>
#include <spdlog/spdlog.h>
#include <types.hpp>

using namespace jm;
using namespace jm::net;

void logHexDump(const std::span<jm::u8> &buf);

static constexpr std::string_view CONNECTION_PREFACE =
    "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static const int MIN_FRAME_SIZE = 9;

static const jm::u32 STREAM_MASK = 017777777777;

struct FrameBits {
   u8 length[24];
   u8 type;
   u8 flags;
   u32 reserved : 1;
   u32 streamId : 31;
   u8 payload[1];
};

enum class FrameType : uint8_t {
   Data = 0x0,
   Headers = 0x1,
   Priority = 0x2,
   RstStream = 0x3,
   Settings = 0x4,
   PushPromise = 0x5,
   Ping = 0x6,
   GoAway = 0x7,
   WindowUpdate = 0x8,
   Continuation = 0x9,
   AltSvc = 0xa,
   Origin = 0xc,
};

union HeaderFlags {
   struct {
      u8 endStream : 1;
      u8 unused0 : 1;
      u8 endHeaders : 1;
      u8 padded : 1;
      u8 unused1 : 1;
      u8 priority : 1;
      u8 unused2 : 2;
   };
   u8 bits;
};

union SettingsFlags {
   struct {
      u8 ack : 1;
      u8 unused : 7;
   };
   u8 bits;
};

struct Http2ChannelHandler::Frame {
   u32 length;
   FrameType type;
   u8 flags;
   u32 streamId;
   std::span<u8> data;
   const ChannelHandlerContext &context;
};

void Http2ChannelHandler::onChannelRead(const net::ChannelHandlerContext &ctx,
                                        const std::span<u8> &buf) {
   // TODO: make faster by removing the scope of this lock.
   std::scoped_lock lock(mutex_);

   if (buf.size() > 0) {

      switch (state_) {
      case State::FAILED:
         return;
      case State::CONNECTION:
         onChannelReadConnectionState(ctx, buf);
         break;
      case State::READING:
         onChannelReadReadingState(ctx, buf);
         break;
      default:
         throw std::runtime_error("unexpected state");
      }
   }

   ctx.next(buf);
}

void Http2ChannelHandler::onChannelReadConnectionState(
    const net::ChannelHandlerContext &ctx, const std::span<u8> &buf) {

   std::copy(buf.begin(), buf.end(), std::back_inserter(buffer_));

   if (buffer_.size() < CONNECTION_PREFACE.size()) {
      return;
   }
   if (std::equal(CONNECTION_PREFACE.begin(), CONNECTION_PREFACE.end(),
                  buffer_.begin())) {
      spdlog::info("protocol is http/2");
      sendInitialSettingsFrame(ctx);
      std::copy(buffer_.begin() + CONNECTION_PREFACE.size(), buffer_.end(),
                buffer_.begin());
      buffer_.resize(buffer_.size() - CONNECTION_PREFACE.size());
      state_ = State::READING;
      processFrames(ctx);
   } else {
      spdlog::info("protocol is not http/2");
      ctx.getPipeline().getChannel().close();
      state_ = State::FAILED;
   }
}

void Http2ChannelHandler::onChannelReadReadingState(
    const net::ChannelHandlerContext &ctx, const std::span<u8> &buf) {

   std::copy(buf.begin(), buf.end(), std::back_inserter(buffer_));

   processFrames(ctx);
}

void Http2ChannelHandler::processFrames(const net::ChannelHandlerContext &ctx) {
   auto iterator = buffer_.begin();
   int distance;
   while ((distance = std::distance(iterator, buffer_.end())) >=
          MIN_FRAME_SIZE) {
      iterator = processNextFrame(ctx, iterator);
   }
   std::copy(iterator, buffer_.end(), buffer_.begin());
   buffer_.resize(buffer_.end() - iterator);
}

std::vector<u8>::iterator
Http2ChannelHandler::processNextFrame(const net::ChannelHandlerContext &ctx,
                                      std::vector<u8>::iterator iterator) {
   // assume there's at least an entire frame here
   const u8 *p = iterator.base();
   Frame frame{.context = ctx};
   frame.length = net::deserialize<u32>(std::span<u8>(iterator.base(), 3));
   u32 bytes_remaining = buffer_.end() - iterator;
   if (frame.length + MIN_FRAME_SIZE > bytes_remaining) {
      // the entire frame has not arrived yet
      return iterator;
   }
   frame.type = static_cast<FrameType>(iterator[3]);
   frame.flags = iterator[4];
   frame.streamId =
       net::deserialize<u32>(std::span<u8>(iterator + 5, iterator + 9)) &
       STREAM_MASK;
   frame.data = std::span<u8>(iterator + MIN_FRAME_SIZE,
                              iterator + frame.length + MIN_FRAME_SIZE);

   spdlog::info("frame: len={} type={} id={} flags={}", frame.length,
                (int)frame.type, frame.streamId, frame.flags);
   switch (frame.type) {
   case FrameType::Settings:
      processFrameSettings(frame);
      break;
   case FrameType::WindowUpdate:
      processFrameWindowUpdate(frame);
      break;
   case FrameType::Headers:
      processFrameHeaders(frame);
      break;
   case FrameType::Data:
      processFrameData(frame);
      break;
   default:
      // skip over for now
      break;
   }
   iterator += frame.length + MIN_FRAME_SIZE;
   return iterator;
}

void Http2ChannelHandler::sendInitialSettingsFrame(
    const ChannelHandlerContext &context) {

   std::vector<u8> data;
   ser::serialize<3>(data, 0);
   ser::serialize<1>(data, static_cast<u8>(FrameType::Settings));
   ser::serialize<1>(data, 0);
   ser::serialize<4>(data, 0);

   enum class SettingsFrameKeys : u16 {
      HeaderTableSize = 0x01,
      EnablePush = 0x02,
      MaxConcurrentStreams = 0x03,
      InitialWindowSize = 0x04,
      MaxFrameSize = 0x05,
      MaxHeaderListSize = 0x06,
   };

   // ser::serialize<2>(data, SettingsFrameKeys::HeaderTableSize);
   // ser::serialize<4>(data, 65536);
   // ser::serialize<2>(data, SettingsFrameKeys::EnablePush);
   // ser::serialize<4>(data, 0);
   // ser::serialize<2>(data, SettingsFrameKeys::MaxConcurrentStreams);
   // ser::serialize<4>(data, 64);
   // ser::serialize<2>(data, SettingsFrameKeys::InitialWindowSize);
   // ser::serialize<4>(data, 65536);
   // ser::serialize<2>(data, SettingsFrameKeys::MaxConcurrentStreams);
   // ser::serialize<4>(data, 1024);
   // ser::serialize<2>(data, SettingsFrameKeys::MaxFrameSize);
   // ser::serialize<4>(data, 65536);

   write(context, data);
}

void Http2ChannelHandler::processFrameSettings(const Frame &frame) {

   SettingsFlags flags{frame.flags};
   if (flags.ack == 0) {
      flags.ack = 1;

      std::vector<u8> data;
      ser::serialize<3>(data, 0);
      ser::serialize<1>(data, static_cast<u8>(FrameType::Settings));
      ser::serialize<1>(data, flags.bits);
      ser::serialize<4>(data, 0);

      write(frame.context, data);
   }
}

void Http2ChannelHandler::processFrameWindowUpdate(const Frame &frame) {}

void Http2ChannelHandler::processFrameData(const Frame &frame) {}

void Http2ChannelHandler::processFrameHeaders(const Frame &frame) {
   if (channels_.find(frame.streamId) != channels_.end()) {
      throw std::runtime_error("protocol error: stream already exists.");
   }

   HeaderFlags flags;

   static_assert(sizeof(flags) == 1);

   flags.bits = frame.flags;
   auto it = frame.data.begin();
   int padLength{};
   if (flags.padded) {
      padLength = *it++;
   }
   bool exclusive{};
   u32 streamDependency{};
   u8 weight{};
   if (flags.priority) {
      u32 value = net::deserialize<u32>(it);
      exclusive = value >> 31;
      streamDependency = value & STREAM_MASK;
      weight = *it++;
   }

   auto channel =
       std::make_unique<http::Http2Channel>(frame.streamId, frame.context);

   auto headers = channel->decodeHeaders(std::span<u8>(it, frame.data.end()));

   for (auto &&item : headers) {
      spdlog::info("{} = {}", item.first, item.second);
   }

   auto request = http::Request{std::move(headers)};
   auto response = dispatcher_(request);

   this->channels_.insert(std::make_pair(frame.streamId, std::move(channel)));

   sendResponse(frame.context, *channel, request, response);

   spdlog::info("headers pri={} padded={} endHeaders={} endStream={}",
                (int)flags.priority, (int)flags.padded, (int)flags.endHeaders,
                (int)flags.endStream);
}

void Http2ChannelHandler::sendResponse(const ChannelHandlerContext &context,
                                       const http::Http2Channel &channel,
                                       const http::Request &request,
                                       const http::Response &response) {
   spdlog::info("sendResponse");

   // encode the status code as a header
   auto statusCode = response.getStatusCode();

   // copy headers (shocking I know)
   auto headers = response.getHeaders();
   headers.emplace(":status", std::to_string(static_cast<int>(statusCode)));
   encodeHeaderFrame(context, headers);
}

void Http2ChannelHandler::encodeHeaderFrame(
    const ChannelHandlerContext &context, const http::HeaderMap &headers) {
   std::vector<u8> frame;

   // initialize header flags
   HeaderFlags flags{};
   flags.endHeaders = 1;
   flags.endStream = 1;

   // find the channel and compress the heders
   auto channelIt = channels_.find(1);
   if (channelIt == channels_.end()) {
      throw std::runtime_error("unable to find channel for streamId");
   }
   auto &channel = *channelIt->second;
   std::vector<u8> encodedHeaders = channel.encodeHeaders(headers);

   // encode the frame
   ser::serialize<3>(frame, encodedHeaders.size());
   ser::serialize<1>(frame, static_cast<u8>(FrameType::Headers));
   ser::serialize<1>(frame, flags.bits);
   ser::serialize<4>(frame, 1);
   std::copy(encodedHeaders.begin(), encodedHeaders.end(),
             std::back_inserter(frame));

   // write frame to client
   write(context, frame);
}

// http::Response Http2ChannelHandler::dispatch(const http::Request &request) {
//    return dispatcher_(request);
// }
