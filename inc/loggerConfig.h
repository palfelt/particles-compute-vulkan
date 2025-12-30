#pragma once

#include <spdlog/spdlog.h>

namespace logging
{
void init(spdlog::level::level_enum logLevel = spdlog::level::warn);
}