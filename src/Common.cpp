
#include <chrono>
#include "Common.hpp"

double GetTime()
{
    using Clock = std::chrono::high_resolution_clock;
    static bool Started = false;
    Clock::time_point StartTime;
    if (!Started)
    {
        StartTime = Clock::now();
        Started = true;
        return 0.0;
    }
    return std::chrono::duration_cast<std::chrono::duration<double>>(Clock::now() - StartTime).count();
}
