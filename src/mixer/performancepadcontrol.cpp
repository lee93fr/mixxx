#include "mixer/performancepadcontrol.h"

#include <algorithm>
#include <cmath>

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "mixer/basetrackplayer.h"
#include "moc_performancepadcontrol.cpp"
#include "track/track.h"

namespace {
constexpr std::array<double, 8> kRollSizes{0.125, 0.25, 0.5, 1, 2, 4, 8, 16};
constexpr std::array<double, 8> kLoopSizes{0.25, 0.5, 1, 2, 4, 8, 16, 32};
QString samplerGroup(int index) {
    return QStringLiteral("[Sampler%1]").arg(index + 1);
}
} // namespace

PerformancePadControl::PerformancePadControl(const QString& group, BaseTrackPlayer* player)
        : QObject(player),
          m_group(group),
          m_player(player) {
    m_modeControl = std::make_unique<ControlObject>(ConfigKey(group, "pad_mode"));
    m_modeControl->set(Hotcue);
    connect(m_modeControl.get(), &ControlObject::valueChanged, this, &PerformancePadControl::setMode);
    for (int mode = 0; mode < ModeCount; ++mode) {
        m_modeButtons[mode] = std::make_unique<ControlPushButton>(
                ConfigKey(group, QStringLiteral("pad_mode_%1").arg(mode)));
        connect(m_modeButtons[mode].get(), &ControlObject::valueChanged, this, [this, mode](double pressed) {
            if (pressed > 0) {
                m_modeControl->set(mode);
                setMode(mode);
            }
        });
    }
    m_slicerReady = std::make_unique<ControlObject>(ConfigKey(group, "slicer_ready"));
    m_currentSlice = std::make_unique<ControlObject>(ConfigKey(group, "slicer_current_slice"));
    m_slicerReady->setReadOnly();
    m_currentSlice->setReadOnly();
    for (int i = 0; i < 8; ++i) {
        const auto prefix = QStringLiteral("performance_pad_%1").arg(i + 1);
        m_pads[i] = std::make_unique<ControlPushButton>(ConfigKey(group, prefix));
        m_clear[i] = std::make_unique<ControlPushButton>(ConfigKey(group, prefix + "_clear"));
        m_active[i] = std::make_unique<ControlObject>(ConfigKey(group, prefix + "_active"));
        m_active[i]->setReadOnly();
        connect(m_pads[i].get(), &ControlObject::valueChanged, this, [this, i](double pressed) { pad(i, pressed > 0); });
        connect(m_clear[i].get(), &ControlObject::valueChanged, this, [this, i](double pressed) {
            if (pressed > 0) {
                clearPad(i);
            }
        });
    }
    connect(player, &BaseTrackPlayer::loadingTrack, this, [this] {
        releasePads();
        leaveDomain();
        m_beats.reset();
    });
    connect(player, &BaseTrackPlayer::newTrackLoaded, this, [this] { refresh(); });
    connect(player, &BaseTrackPlayer::trackUnloaded, this, [this] {
        releasePads();
        leaveDomain();
        m_beats.reset();
        m_slicerReady->forceSet(0);
        m_currentSlice->forceSet(0);
    });
    m_timer.setInterval(40);
    connect(&m_timer, &QTimer::timeout, this, &PerformancePadControl::refresh);
    m_timer.start();
}

PerformancePadControl::~PerformancePadControl() {
    releasePads();
    leaveDomain();
}

ControlProxy* PerformancePadControl::control(const QString& group, const QString& key) {
    const auto id = group + QChar(',') + key;
    auto it = m_controls.constFind(id);
    if (it == m_controls.constEnd()) {
        auto* proxy = new ControlProxy(group, key, this);
        m_controls.insert(id, proxy);
        return proxy;
    }
    return it.value();
}
ControlProxy* PerformancePadControl::control(const QString& key) {
    return control(m_group, key);
}
double PerformancePadControl::value(const QString& key) {
    return control(key)->get();
}
void PerformancePadControl::write(const QString& key, double v) {
    control(key)->set(v);
}
void PerformancePadControl::pulse(const QString& group, const QString& key) {
    auto* proxy = control(group, key);
    proxy->set(1);
    proxy->set(0);
}

void PerformancePadControl::setMode(double v) {
    const auto mode = static_cast<Mode>(std::clamp(static_cast<int>(std::round(v)), 0, static_cast<int>(ModeCount) - 1));
    if (mode != m_mode) {
        releasePads();
        leaveDomain();
        m_mode = mode;
        if (mode == Sampler) {
            if (ControlObject::get(ConfigKey("[App]", "num_samplers")) < 8) {
                ControlObject::set(ConfigKey("[App]", "num_samplers"), 8);
            }
            if (ControlObject::exists(ConfigKey("[GameflorrSkin]", "sampler_visible"))) {
                ControlObject::set(ConfigKey("[GameflorrSkin]", "sampler_visible"), 1);
            }
        }
    }
    if (v != static_cast<double>(mode)) {
        m_modeControl->set(mode);
    }
    refresh();
}

void PerformancePadControl::pad(int index, bool pressed) {
    if (m_held[index] == pressed) {
        return;
    }
    m_held[index] = pressed;
    const auto number = index + 1;
    switch (m_mode) {
    case Hotcue:
        write(QStringLiteral("hotcue_%1_activate").arg(number), pressed);
        break;
    case Roll:
        if (pressed && std::count(m_held.begin(), m_held.end(), true) == 1) {
            m_rollSlip = value("slip_enabled") > 0;
            saveLoop();
        }
        write(QStringLiteral("beatlooproll_%1_activate").arg(kRollSizes[index]), pressed);
        if (!pressed && std::none_of(m_held.begin(), m_held.end(), [](bool held) { return held; })) {
            restoreLoop();
            write("slip_enabled", m_rollSlip);
        }
        break;
    case Sampler:
        if (pressed) {
            pulse(samplerGroup(index), "cue_gotoandplay");
        }
        break;
    case Loop:
        write(QStringLiteral("beatloop_%1_toggle").arg(kLoopSizes[index]), pressed);
        break;
    case BeatJump:
        write(QStringLiteral("beatjump_%1_%2")
                        .arg(1 << (index / 2))
                        .arg(index % 2 ? "forward" : "backward"),
                pressed);
        break;
    case Slicer:
    case SlicerLoop:
        if (pressed) {
            if (applySlice(index)) {
                m_sliceStack.push_back(index);
            }
        } else {
            std::erase(m_sliceStack, index);
            if (m_sliceStack.empty()) {
                finishSlice();
            } else {
                applySlice(m_sliceStack.back());
            }
        }
        break;
    case ModeCount:
        break;
    }
    refresh();
}

void PerformancePadControl::clearPad(int index) {
    if (m_mode == Hotcue) {
        pulse(m_group, QStringLiteral("hotcue_%1_clear").arg(index + 1));
    } else if (m_mode == Sampler) {
        pulse(samplerGroup(index), "stop");
    } else if (m_mode == Loop) {
        pulse(m_group, "loop_exit");
    }
    refresh();
}

void PerformancePadControl::releasePads() {
    for (int i = 0; i < 8; ++i) {
        if (m_held[i]) {
            pad(i, false);
        }
        m_pads[i]->set(0);
    }
    finishSlice();
}

void PerformancePadControl::saveLoop() {
    m_previousLoopStart = value("loop_start_position");
    m_previousLoopEnd = value("loop_end_position");
    m_previousLoopEnabled = value("loop_enabled") > 0;
}
void PerformancePadControl::restoreLoop() {
    write("loop_enabled", 0);
    write("loop_start_position", m_previousLoopStart);
    write("loop_end_position", m_previousLoopEnd);
    write("loop_enabled", m_previousLoopEnabled);
}
void PerformancePadControl::setLoop(mixxx::audio::FramePos start, mixxx::audio::FramePos end) {
    write("loop_enabled", 0);
    write("loop_start_position", start.toEngineSamplePos());
    write("loop_end_position", end.toEngineSamplePos());
    write("loop_enabled", 1);
}

bool PerformancePadControl::updateDomain(bool force) {
    const auto track = m_player->getLoadedTrack();
    const auto beats = track ? track->getBeats() : mixxx::BeatsPointer();
    const double samples = value("track_samples");
    if (!beats || samples <= 0) {
        return false;
    }
    if (beats != m_beats) {
        finishSlice();
        leaveDomain();
        m_beats = beats;
        force = true;
    }
    const auto position = mixxx::audio::FramePos::fromEngineSamplePos(value("playposition") * samples);
    if (position < mixxx::audio::kStartFramePos || position.toEngineSamplePos() >= samples) {
        return false;
    }
    if (!m_slicing && !m_domainLoop && (force || !m_domainStart.isValid() || position < m_domainStart || position >= m_domainEnd)) {
        auto start = m_domainStart;
        if (!force && start.isValid() && position >= m_domainEnd &&
                position < beats->findNBeatsFromPosition(m_domainEnd, 8)) {
            start = m_domainEnd;
        } else {
            start = beats->findPrevBeat(position);
        }
        if (!start.isValid()) {
            return false;
        }
        const auto end = beats->findNBeatsFromPosition(start, 8);
        if (!start.isValid() || !end.isValid() || start < mixxx::audio::kStartFramePos ||
                end <= start || end.toEngineSamplePos() > samples) {
            return false;
        }
        m_domainStart = start;
        m_domainEnd = end;
    }
    if (m_mode == SlicerLoop && !m_domainLoop) {
        saveLoop();
        setLoop(m_domainStart, m_domainEnd);
        m_domainLoop = true;
    }
    return m_domainStart.isValid() && m_domainEnd.isValid();
}

bool PerformancePadControl::applySlice(int index) {
    if (!updateDomain()) {
        return false;
    }
    const auto start = m_beats->findNBeatsFromPosition(m_domainStart, index);
    const auto end = m_beats->findNBeatsFromPosition(m_domainStart, index + 1);
    if (!start.isValid() || !end.isValid() || end <= start) {
        return false;
    }
    if (!m_slicing) {
        if (!m_domainLoop) {
            saveLoop();
        }
        m_previousSlip = value("slip_enabled") > 0;
        write("slip_enabled", 1);
        m_slicing = true;
    }
    setLoop(start, end);
    write("playposition", start.toEngineSamplePos() / value("track_samples"));
    return true;
}

void PerformancePadControl::finishSlice() {
    if (!m_slicing) {
        return;
    }
    if (m_domainLoop) {
        setLoop(m_domainStart, m_domainEnd);
    } else {
        restoreLoop();
    }
    write("slip_enabled", m_previousSlip);
    m_slicing = false;
    m_sliceStack.clear();
}
void PerformancePadControl::leaveDomain() {
    if (m_domainLoop) {
        restoreLoop();
        m_domainLoop = false;
    }
    m_domainStart = mixxx::audio::kInvalidFramePos;
    m_domainEnd = mixxx::audio::kInvalidFramePos;
}

void PerformancePadControl::refresh() {
    const bool slicer = m_mode == Slicer || m_mode == SlicerLoop;
    const bool ready = slicer && updateDomain();
    if (slicer && !ready) {
        finishSlice();
        leaveDomain();
    }
    m_slicerReady->forceSet(ready);
    int current = -1;
    if (ready) {
        const auto position = mixxx::audio::FramePos::fromEngineSamplePos(
                value("playposition") * value("track_samples"));
        for (int i = 0; i < 8; ++i) {
            if (position >= m_beats->findNBeatsFromPosition(m_domainStart, i) &&
                    position < m_beats->findNBeatsFromPosition(m_domainStart, i + 1)) {
                current = i;
                break;
            }
        }
    }
    m_currentSlice->forceSet(current + 1);
    for (int i = 0; i < 8; ++i) {
        bool active = false;
        switch (m_mode) {
        case Hotcue:
            active = value(QStringLiteral("hotcue_%1_status").arg(i + 1)) > 0;
            break;
        case Sampler:
            active = control(samplerGroup(i), "play")->toBool();
            break;
        case Loop:
            active = value(QStringLiteral("beatloop_%1_enabled").arg(kLoopSizes[i])) > 0;
            break;
        case Slicer:
        case SlicerLoop:
            active = ready && (m_held[i] || current == i);
            break;
        default:
            active = m_held[i];
            break;
        }
        m_active[i]->forceSet(active);
    }
}
