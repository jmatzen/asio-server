#include <acceptor.hpp>
#include <io_context.hpp>

using namespace jm;

Acceptor::Acceptor(IoContext &context, int port)
    : acceptor_(context.get(), tcp::endpoint(tcp::v4(), port)) {}