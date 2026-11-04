#pragma once

#include <array>

#include "control/pollingcontrolproxy.h"
#include "gameflorr/transitionplan.h"

namespace gameflorr {
class StemMashup {
  public:
    bool begin(const QString& from, const QString& to) {
        restore();
        if (from == to)
            return false;
        for (int i = 0; i < 8; ++i) {
            QString group = i < 4 ? from : to;
            group.chop(1);
            group += QStringLiteral("_Stem%1]").arg(i % 4 + 1);
            PollingControlProxy volume(group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            PollingControlProxy mute(group, QStringLiteral("mute"), ControlFlag::AllowMissingOrInvalid);
            if (!volume.valid() || !mute.valid() || mute.toBool() ||
                    !isFinite(volume.get()) || std::abs(volume.get() - 1) > 1e-6)
                return false;
            m_channels[i] = {group, volume.get(), volume.get()};
        }
        m_active = true;
        update(0);
        return true;
    }
    void update(double progress) {
        if (!m_active)
            return;
        for (const auto& channel : m_channels) {
            PollingControlProxy volume(channel.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            PollingControlProxy mute(channel.group, QStringLiteral("mute"), ControlFlag::AllowMissingOrInvalid);
            if (!isFinite(progress) || !volume.valid() || !mute.valid() || mute.toBool() || !isFinite(volume.get()) ||
                    std::abs(volume.get() - channel.last) > 1e-6) {
                restore();
                return;
            }
        }
        const double backing = 1 - std::clamp(progress / 0.4, 0.0, 1.0);
        const double vocalSwap = std::clamp((progress - 0.65) / 0.35, 0.0, 1.0);
        for (int i = 0; i < 8; ++i) {
            auto& channel = m_channels[i];
            const double gain = i < 3 ? backing : i == 3 ? 1 - vocalSwap
                    : i == 7                             ? vocalSwap
                                                         : 1;
            channel.last = channel.original * gain;
            PollingControlProxy(channel.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid).set(channel.last);
        }
    }
    void restore() {
        if (!m_active)
            return;
        for (const auto& channel : m_channels) {
            PollingControlProxy volume(channel.group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            if (volume.valid() && std::abs(volume.get() - channel.last) < 1e-6)
                volume.set(channel.original);
        }
        m_active = false;
    }
    bool active() const {
        return m_active;
    }

  private:
    struct Channel {
        QString group;
        double original = 1;
        double last = 1;
    };
    std::array<Channel, 8> m_channels;
    bool m_active = false;
};
} // namespace gameflorr
