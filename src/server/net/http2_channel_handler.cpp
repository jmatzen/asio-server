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
   case FrameType::Ping:
      // Echo PING frames back
      if (frame.data.size() == 8) {
         std::vector<u8> pongFrame;
         ser::serialize<3>(pongFrame, 8);
         ser::serialize<1>(pongFrame, static_cast<u8>(FrameType::Ping));
         ser::serialize<1>(pongFrame, 0x01); // ACK flag
         ser::serialize<4>(pongFrame, 0); // Stream ID 0
         std::copy(frame.data.begin(), frame.data.end(), std::back_inserter(pongFrame));
         write(ctx, pongFrame);
      }
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
   
   enum class SettingsFrameKeys : u16 {
      HeaderTableSize = 0x01,
      EnablePush = 0x02,
      MaxConcurrentStreams = 0x03,
      InitialWindowSize = 0x04,
      MaxFrameSize = 0x05,
      MaxHeaderListSize = 0x06,
   };

   // Calculate settings frame length (6 bytes per setting)
   u32 settingsLength = 6 * 4; // 4 settings
   
   // Frame header
   ser::serialize<3>(data, settingsLength);
   ser::serialize<1>(data, static_cast<u8>(FrameType::Settings));
   ser::serialize<1>(data, 0); // No flags
   ser::serialize<4>(data, 0); // Stream ID 0 for connection-level settings

   // Settings payload
   ser::serialize<2>(data, static_cast<u16>(SettingsFrameKeys::EnablePush));
   ser::serialize<4>(data, 0); // Disable server push
   
   ser::serialize<2>(data, static_cast<u16>(SettingsFrameKeys::MaxConcurrentStreams));
   ser::serialize<4>(data, 100); // Max concurrent streams
   
   ser::serialize<2>(data, static_cast<u16>(SettingsFrameKeys::InitialWindowSize));
   ser::serialize<4>(data, initialStreamWindowSize_); // Initial window size
   
   ser::serialize<2>(data, static_cast<u16>(SettingsFrameKeys::MaxFrameSize));
   ser::serialize<4>(data, 16384); // Max frame size (16KB)

   write(context, data);
}

void Http2ChannelHandler::processFrameSettings(const Frame &frame) {
   SettingsFlags flags{frame.flags};
   
   if (flags.ack == 0) {
      // Process settings values
      auto data = frame.data;
      while (data.size() >= 6) { // Each setting is 6 bytes
         u16 id = net::deserialize<u16>(data.subspan(0, 2));
         u32 value = net::deserialize<u32>(data.subspan(2, 4));
         data = data.subspan(6);
         
         switch (id) {
            case 0x04: // INITIAL_WINDOW_SIZE
               if (value > 0x7FFFFFFF) {
                  spdlog::error("Invalid initial window size: {}", value);
                  // TODO: Send GOAWAY frame
                  return;
               }
               // Update existing streams' window sizes
               i32 delta = static_cast<i32>(value) - static_cast<i32>(initialStreamWindowSize_);
               for (auto& [streamId, windowSize] : streamWindowSizes_) {
                  updateStreamWindow(streamId, delta);
               }
               initialStreamWindowSize_ = value;
               spdlog::info("Updated initial window size to {}", value);
               break;
            // Handle other settings as needed
         }
      }
      
      // Send settings ACK
      flags.ack = 1;
      std::vector<u8> ackData;
      ser::serialize<3>(ackData, 0);
      ser::serialize<1>(ackData, static_cast<u8>(FrameType::Settings));
      ser::serialize<1>(ackData, flags.bits);
      ser::serialize<4>(ackData, 0);

      write(frame.context, ackData);
   }
   // If ACK flag is set, this is an acknowledgment of our settings
}

void Http2ChannelHandler::processFrameWindowUpdate(const Frame &frame) {
   if (frame.data.size() != 4) {
      spdlog::error("Invalid WINDOW_UPDATE frame size: {}", frame.data.size());
      return;
   }

   u32 windowSizeIncrement = net::deserialize<u32>(frame.data);
   
   // Check for invalid increment (must be 1-2^31-1)
   if (windowSizeIncrement == 0 || windowSizeIncrement > 0x7FFFFFFF) {
      spdlog::error("Invalid window size increment: {}", windowSizeIncrement);
      // TODO: Send GOAWAY frame
      return;
   }

   if (frame.streamId == 0) {
      // Connection-level window update
      updateConnectionWindow(windowSizeIncrement);
      spdlog::info("Connection window updated by {}, new size: {}", 
                   windowSizeIncrement, connectionWindowSize_);
   } else {
      // Stream-level window update
      updateStreamWindow(frame.streamId, windowSizeIncrement);
      spdlog::info("Stream {} window updated by {}", 
                   frame.streamId, windowSizeIncrement);
   }
}

void Http2ChannelHandler::processFrameData(const Frame &frame) {
   if (frame.streamId == 0) {
      spdlog::error("DATA frame with stream ID 0 is not allowed");
      // TODO: Send PROTOCOL_ERROR
      return;
   }

   // Check if stream exists
   auto channelIt = channels_.find(frame.streamId);
   if (channelIt == channels_.end()) {
      spdlog::error("DATA frame for unknown stream {}", frame.streamId);
      // TODO: Send RST_STREAM
      return;
   }

   u32 dataLength = frame.data.size();
   
   // Handle padding if present
   union DataFlags {
      struct {
         u8 endStream : 1;
         u8 unused0 : 2;
         u8 padded : 1;
         u8 unused1 : 4;
      };
      u8 bits;
   };
   
   DataFlags flags{frame.flags};
   u32 padLength = 0;
   auto dataStart = frame.data.begin();
   
   if (flags.padded) {
      if (frame.data.empty()) {
         spdlog::error("DATA frame with PADDED flag but no data");
         return;
      }
      padLength = frame.data[0];
      dataStart++;
      dataLength--;
   }
   
   if (padLength >= dataLength) {
      spdlog::error("Invalid padding length {} for data length {}", padLength, dataLength);
      return;
   }
   
   u32 actualDataLength = dataLength - padLength;
   
   // Update flow control windows
   if (actualDataLength > 0) {
      consumeConnectionWindow(actualDataLength);
      consumeStreamWindow(frame.streamId, actualDataLength);
      
      // Send window updates if needed (simple threshold-based approach)
      if (connectionWindowSize_ < 32768) { // Half of initial window
         sendWindowUpdate(frame.context, 0, 32768);
      }
      
      auto streamWindowIt = streamWindowSizes_.find(frame.streamId);
      if (streamWindowIt != streamWindowSizes_.end() && streamWindowIt->second < 32768) {
         sendWindowUpdate(frame.context, frame.streamId, 32768);
      }
   }
   
   // TODO: Process actual data payload
   // For now, just log that we received data
   spdlog::info("Received {} bytes of data on stream {}, endStream={}", 
                actualDataLength, frame.streamId, (int)flags.endStream);
   
   if (flags.endStream) {
      // Stream is half-closed (remote)
      // TODO: Implement proper stream state management
      spdlog::info("Stream {} half-closed (remote)", frame.streamId);
   }
}

void Http2ChannelHandler::processFrameHeaders(const Frame &frame) {
   if (channels_.find(frame.streamId) != channels_.end()) {
      throw std::runtime_error("protocol error: stream already exists.");
   }

   // Initialize stream window size
   streamWindowSizes_[frame.streamId] = initialStreamWindowSize_;

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

   sendResponse(frame.context, *channels_[frame.streamId], request, response);

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
   encodeHeaderFrame(context, headers, channel.getStreamId());
}

void Http2ChannelHandler::encodeHeaderFrame(
    const ChannelHandlerContext &context, const http::HeaderMap &headers, u32 streamId) {
   std::vector<u8> frame;

   // initialize header flags
   HeaderFlags flags{};
   flags.endHeaders = 1;
   flags.endStream = 1;

   // find the channel and compress the heders
   auto channelIt = channels_.find(streamId);
   if (channelIt == channels_.end()) {
      throw std::runtime_error("unable to find channel for streamId");
   }
   auto &channel = *channelIt->second;
   std::vector<u8> encodedHeaders = channel.encodeHeaders(headers);

   // encode the frame
   ser::serialize<3>(frame, encodedHeaders.size());
   ser::serialize<1>(frame, static_cast<u8>(FrameType::Headers));
   ser::serialize<1>(frame, flags.bits);
   ser::serialize<4>(frame, streamId);
   std::copy(encodedHeaders.begin(), encodedHeaders.end(),
             std::back_inserter(frame));

   // write frame to client
   write(context, frame);
}

// Flow control helper methods
void Http2ChannelHandler::updateConnectionWindow(i32 delta) {
   i64 newSize = static_cast<i64>(connectionWindowSize_) + delta;
   if (newSize > 0x7FFFFFFF) {
      spdlog::error("Connection window size overflow");
      // TODO: Send GOAWAY frame
      return;
   }
   connectionWindowSize_ = static_cast<u32>(std::max(0LL, newSize));
}

void Http2ChannelHandler::updateStreamWindow(u32 streamId, i32 delta) {
   auto it = streamWindowSizes_.find(streamId);
   if (it == streamWindowSizes_.end()) {
      spdlog::error("Attempting to update window for unknown stream {}", streamId);
      return;
   }
   
   i64 newSize = static_cast<i64>(it->second) + delta;
   if (newSize > 0x7FFFFFFF) {
      spdlog::error("Stream {} window size overflow", streamId);
      // TODO: Send RST_STREAM frame
      return;
   }
   it->second = static_cast<u32>(std::max(0LL, newSize));
}

void Http2ChannelHandler::sendWindowUpdate(const ChannelHandlerContext& context, 
                                           u32 streamId, u32 increment) {
   std::vector<u8> frame;
   
   // Frame header
   ser::serialize<3>(frame, 4); // WINDOW_UPDATE payload is always 4 bytes
   ser::serialize<1>(frame, static_cast<u8>(FrameType::WindowUpdate));
   ser::serialize<1>(frame, 0); // No flags
   ser::serialize<4>(frame, streamId);
   
   // Payload: window size increment
   ser::serialize<4>(frame, increment);
   
   write(context, frame);
   
   // Update our local window size
   if (streamId == 0) {
      updateConnectionWindow(increment);
   } else {
      updateStreamWindow(streamId, increment);
   }
}

bool Http2ChannelHandler::canSendData(u32 streamId, u32 dataSize) {
   if (dataSize > connectionWindowSize_) {
      return false;
   }
   
   auto it = streamWindowSizes_.find(streamId);
   if (it == streamWindowSizes_.end()) {
      return false;
   }
   
   return dataSize <= it->second;
}

void Http2ChannelHandler::consumeConnectionWindow(u32 dataSize) {
   if (dataSize > connectionWindowSize_) {
      spdlog::error("Attempting to consume more data than available in connection window");
      connectionWindowSize_ = 0;
   } else {
      connectionWindowSize_ -= dataSize;
   }
}

void Http2ChannelHandler::consumeStreamWindow(u32 streamId, u32 dataSize) {
   auto it = streamWindowSizes_.find(streamId);
   if (it == streamWindowSizes_.end()) {
      spdlog::error("Attempting to consume window for unknown stream {}", streamId);
      return;
   }
   
   if (dataSize > it->second) {
      spdlog::error("Attempting to consume more data than available in stream {} window", streamId);
      it->second = 0;
   } else {
      it->second -= dataSize;
   }
}

// http::Response Http2ChannelHandler::dispatch(const http::Request &request) {
//    return dispatcher_(request);
// }
