#pragma once

#include <array>

#include "control/pollingcontrolproxy.h"
#include "gameflorr/transitionplan.h"

namespace gameflorr {
inline bool liveMashupTempoCompatible(double a, double b) {
    return isFinite(a) && isFinite(b) && a >= 40 && b >= 40 &&
            std::abs(a / b - 1) <= 0.005;
}

class LiveMashup {
  public:
    bool select(int mode) {
        if (mode != 1 && mode != 2) {
            return false;
        }
        if (!busy()) {
            std::array<Channel, 8> channels;
            for (int i = 0; i < 8; ++i) {
                const QString group = QStringLiteral("[Channel%1_Stem%2]")
                                              .arg(i / 4 + 1)
                                              .arg(i % 4 + 1);
                PollingControlProxy volume(group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
                PollingControlProxy mute(group, QStringLiteral("mute"), ControlFlag::AllowMissingOrInvalid);
                if (!volume.valid() || !mute.valid() || mute.toBool() ||
                        !isFinite(volume.get()) || volume.get() < 0 || volume.get() > 1) {
                    return false;
                }
                channels[i] = {group, volume.get(), volume.get(), volume.get(), volume.get(), true};
            }
            m_channels = channels;
        } else if (!checkOwnership()) {
            return false;
        }
        m_mode = mode;
        for (int i = 0; i < 8; ++i) {
            auto& c = m_channels[i];
            c.start = c.last;
            const bool vocalDeck = i / 4 == mode - 1;
            c.target = vocalDeck == (i % 4 == 3) ? c.original : 0;
        }
        m_elapsed = 0;
        return true;
    }

    void stop() {
        m_mode = 0;
        for (auto& c : m_channels) {
            c.start = c.last;
            c.target = c.original;
        }
        m_elapsed = 0;
    }

    void abandonDeck(int deck) {
        for (int i = deck * 4; i < deck * 4 + 4; ++i) {
            m_channels[i].owned = false;
        }
    }

    void advance(double seconds) {
        if (!busy() || !isFinite(seconds) || seconds < 0) {
            return;
        }
        checkOwnership();
        m_elapsed += seconds;
        const double progress = std::clamp(m_elapsed / 0.25, 0.0, 1.0);
        for (auto& c : m_channels) {
            if (!c.owned) {
                continue;
            }
            c.last = c.start + (c.target - c.start) * progress;
            PollingControlProxy(c.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid).set(c.last);
            if (!m_mode && progress >= 1) {
                c.owned = false;
            }
        }
    }

    void restore() {
        for (auto& c : m_channels) {
            if (!c.owned) {
                continue;
            }
            PollingControlProxy volume(c.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            if (volume.valid() && isFinite(volume.get()) && std::abs(volume.get() - c.last) < 1e-6) {
                volume.set(c.original);
            }
            c.owned = false;
        }
        m_mode = 0;
    }

    int mode() const {
        return m_mode;
    }
    bool busy() const {
        return std::any_of(m_channels.begin(), m_channels.end(), [](const Channel& c) { return c.owned; });
    }

  private:
    struct Channel {
        QString group;
        double original = 1;
        double last = 1;
        double start = 1;
        double target = 1;
        bool owned = false;
    };
    bool checkOwnership() {
        bool interrupted = false;
        for (auto& c : m_channels) {
            if (!c.owned) {
                continue;
            }
            PollingControlProxy volume(c.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            PollingControlProxy mute(c.group, QStringLiteral("mute"), ControlFlag::AllowMissingOrInvalid);
            if (!volume.valid() || !isFinite(volume.get()) || std::abs(volume.get() - c.last) > 1e-6) {
                c.owned = false;
                interrupted = true;
            }
            if (!mute.valid() || mute.toBool()) {
                interrupted = true;
            }
        }
        if (interrupted && m_mode) {
            stop();
        }
        return !interrupted;
    }
    std::array<Channel, 8> m_channels;
    int m_mode = 0;
    double m_elapsed = 0;
};
} // namespace gameflorr
