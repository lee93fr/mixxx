#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

namespace gameflorr {
inline bool isFinite(double value) {
    return (std::bit_cast<std::uint64_t>(value) &
                   UINT64_C(0x7ff0000000000000)) !=
            UINT64_C(0x7ff0000000000000);
}
struct TransitionPlan {
    double seconds;
    int bars;
    bool sync;
    bool bassSwap;
};
inline TransitionPlan planTransition(bool creative,
        double intensity,
        double fromBpm,
        double toBpm,
        bool stableGrids,
        double outroSeconds,
        double introSeconds) {
    if (!creative) {
        return {10.0, 0, false, false};
    }
    if (!isFinite(fromBpm) || !isFinite(toBpm) || fromBpm < 40 || toBpm < 40) {
        return {8.0, 0, false, false};
    }
    const double ratio = fromBpm / toBpm;
    if (ratio < 0.94 || ratio > 1.06) {
        return {3.0, 0, false, false};
    }
    if (!stableGrids || !isFinite(outroSeconds) || !isFinite(introSeconds) ||
            outroSeconds <= 0 || introSeconds <= 0) {
        return {8.0, 0, false, false};
    }
    const int desiredBars = intensity >= 0.75 ? 32 : intensity >= 0.35 ? 16 : 8;
    const double secondsPerBar = 240.0 / fromBpm;
    const double available = std::min(outroSeconds, introSeconds);
    int bars = desiredBars;
    while (bars >= 8 && bars * secondsPerBar > available) {
        bars /= 2;
    }
    if (bars < 8) {
        return {std::min(8.0, available), 0, false, false};
    }
    return {bars * secondsPerBar, bars, true, intensity >= 0.35};
}
} // namespace gameflorr
