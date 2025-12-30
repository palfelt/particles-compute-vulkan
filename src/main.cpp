#include <cstdlib>

#include "loggerConfig.h"
#include "tApp.h"

void initLogging()
{
    const auto logLevel = spdlog::level::info;
    logging::init(logLevel);
    spdlog::set_level(logLevel);
}

int main()
{
    initLogging();

    tApp app{};

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        spdlog::critical("main: Exited app due to exception: {}", e.what());
        return EXIT_FAILURE;
    }

    spdlog::info("main: Exited app successully");
    return EXIT_SUCCESS;
}