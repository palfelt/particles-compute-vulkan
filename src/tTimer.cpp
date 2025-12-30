#include "tTimer.h"

float tTimer::tick()
{
    const auto now = Clock::now();
    std::chrono::duration<float> dt = now - Last;
    Last = now;

    Elapsed += dt.count();
    return dt.count();
}