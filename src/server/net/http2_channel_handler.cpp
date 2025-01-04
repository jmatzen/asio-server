#include <net/channel.hpp>
#include <net/channel_handler_context.hpp>
#include <net/channel_pipeline.hpp>
#include <net/deserialize.hpp>
#include <net/hpack.hpp>
#include <net/http2_channel_handler.hpp>
#include <spdlog/spdlog.h>
#include <types.hpp>

using namespace jm;
using namespace jm::net;

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

struct Http2ChannelHandler::Frame {
   u32 length;
   FrameType type;
   u8 flags;
   u32 streamId;
   std::span<u8> data;
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
      std::copy(buffer_.begin() + CONNECTION_PREFACE.size(), buffer_.end(),
                buffer_.begin());
      buffer_.resize(buffer_.size() - CONNECTION_PREFACE.size());
      state_ = State::READING;
      processFrames();
   } else {
      spdlog::info("protocol is not http/2");
      ctx.getPipeline().getChannel().close();
      state_ = State::FAILED;
   }
}

void Http2ChannelHandler::onChannelReadReadingState(
    const net::ChannelHandlerContext &ctx, const std::span<u8> &buf) {

   std::copy(buf.begin(), buf.end(), std::back_inserter(buffer_));

   processFrames();
}

void Http2ChannelHandler::processFrames() {
   auto iterator = buffer_.begin();
   int distance;
   while ((distance = std::distance(iterator, buffer_.end())) >=
          MIN_FRAME_SIZE) {
      iterator = processNextFrame(iterator);
   }
   std::copy(iterator, buffer_.end(), buffer_.begin());
   buffer_.resize(buffer_.end() - iterator);
}

std::vector<u8>::iterator
Http2ChannelHandler::processNextFrame(std::vector<u8>::iterator iterator) {
   // assume there's at least an entire frame here
   const u8 *p = iterator.base();
   Frame frame{};
   frame.length = net::deserialize<u32>(
       std::span<u8>(iterator.base(), 3)); // p[0] << 16 | p[1] << 8 | p[2];
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
   default:
      // skip over for now
      break;
   }
   iterator += frame.length + MIN_FRAME_SIZE;
   return iterator;
}

void Http2ChannelHandler::processFrameSettings(const Frame &frame) {}

void Http2ChannelHandler::processFrameWindowUpdate(const Frame &frame) {}

void Http2ChannelHandler::processFrameHeaders(const Frame &frame) {
   union {
      struct {
         u8 endStream : 1;
         u8 unused0 : 1;
         u8 endHeaders : 1;
         u8 padded : 1;
         u8 unused1 : 1;
         u8 priority : 1;
         u8 unused2 : 2;

      } flags;
      u8 bits;

   } flags;

   static_assert(sizeof(flags) == 1);

   flags.bits = frame.flags;
   auto it = frame.data.begin();
   int padLength{};
   if (flags.flags.padded) {
      padLength = *it++;
   }
   bool exclusive{};
   u32 streamDependency{};
   u8 weight{};
   if (flags.flags.priority) {
      u32 value = net::deserialize<u32>(it);
      exclusive = value >> 31;
      streamDependency = value & STREAM_MASK;
      weight = *it++;
   }

   HPack hpack;
   auto len = frame.data.end() - it;
   auto headers = hpack.decode(std::span<u8>(it, frame.data.end()));
   for (auto &&item : headers) {
      spdlog::info("{} = {}", item.first, item.second);
   }

   spdlog::info("headers pri={} padded={} endHeaders={} endStream={}",
                (int)flags.flags.priority, (int)flags.flags.padded,
                (int)flags.flags.endHeaders, (int)flags.flags.endStream);
}

// hpack helpers
extern "C" void freezero(void *p, size_t len) {
   memset(p, 0, len);
   free(p);
}

extern "C" void *recallocarray(void *p, size_t o, size_t n, size_t size) {
   return realloc(p, n * size);
}
