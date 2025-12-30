#include "loggerConfig.h"

#include <spdlog/sinks/stdout_color_sinks.h>

void logging::init(spdlog::level::level_enum logLevel)
{
    auto logger = spdlog::stdout_color_mt("global");
    logger->set_level(logLevel);
    spdlog::set_default_logger(logger);
    spdlog::set_level(logLevel);
}