#include <net/channel_handler_context.hpp>
#include <net/dump_channel_handler.hpp>
#include <spdlog/spdlog.h>

using namespace jm;
using namespace jm::net;

void logHexDump(const char* prefix, const std::span<jm::u8>& buf) {
   for (int offset = 0; offset < buf.size(); offset += 16)
   {
      std::string line;
      line += fmt::format("{} {:04x} ", prefix, offset);
      for (int q = offset; q < offset + 16; ++q)
      {
         if (q < buf.size())
         {
            line += fmt::format("{:02x} ", u8(buf[q]));
         }
         else
         {
            line += "   ";
         }
      }
      line += " ";
      for (int q = offset; q < offset + 16 && q < buf.size(); ++q)
      {
         if (q < buf.size())
         {
            line += buf[q] >= ' ' && buf[q] <= 127 ? buf[q] : '.';
         }
      }

      spdlog::info(line);
   }

}

void DumpChannelHandler::onChannelRead(const net::ChannelHandlerContext &ctx, const std::span<u8> &buf)
{
   logHexDump("<", buf);
   ctx.next(buf);
}

void DumpChannelHandler::write(const net::ChannelHandlerContext &ctx, const std::span<u8> &buf) const {
   logHexDump(">", buf);
   ChannelHandler::write(ctx, buf);
}