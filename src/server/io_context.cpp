#include <io_context.hpp>
#include <spdlog/spdlog.h>

void jm::IoContext::handleException(const std::exception &e) {
    spdlog::warn("capture exception with timer handler {} (cancelled)",
                 e.what());
}