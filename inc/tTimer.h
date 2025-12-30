#pragma once
#include <chrono>

class tTimer
{
  public:
    tTimer() : Last(Clock::now()) {}

    float tick();
    float getElapsed() const { return Elapsed; }

  private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point Last;
    float Elapsed{0.f};
};