#include <net/channel_handler_context.hpp>
#include <net/dump_channel_handler.hpp>
#include <spdlog/spdlog.h>

using namespace jm::net;

void DumpChannelHandler::onChannelRead(const net::ChannelHandlerContext &ctx, const std::span<u8> &buf)
{
   std::string s((char *)buf.data(), buf.size());
   for (int offset = 0; offset < buf.size(); offset += 16)
   {
      std::string line;
      line += fmt::format("{:04x} ", offset);
      for (int q = offset; q < offset + 16; ++q)
      {
         if (q < buf.size())
         {
            line += fmt::format("{:02x} ", u8(s[q]));
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
            line += s[q] >= ' ' && s[q] <= 127 ? s[q] : '.';
         }
      }

      spdlog::info(line);
   }
   ctx.next(buf);
}
