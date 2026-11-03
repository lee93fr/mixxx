#pragma once

#include <QHash>
#include <QObject>
#include <QTimer>
#include <array>
#include <memory>
#include <vector>

#include "track/beats.h"

class BaseTrackPlayer;
class ControlObject;
class ControlProxy;
class ControlPushButton;

class PerformancePadControl : public QObject {
    Q_OBJECT
  public:
    enum Mode { Hotcue,
        Roll,
        Slicer,
        Sampler,
        Loop,
        BeatJump,
        SlicerLoop,
        ModeCount };
    PerformancePadControl(const QString& group, BaseTrackPlayer* player);
    ~PerformancePadControl() override;

  private:
    ControlProxy* control(const QString& group, const QString& key);
    ControlProxy* control(const QString& key);
    double value(const QString& key);
    void write(const QString& key, double value);
    void pulse(const QString& group, const QString& key);
    void setMode(double value);
    void pad(int index, bool pressed);
    void clearPad(int index);
    void releasePads();
    void refresh();
    bool updateDomain(bool force = false);
    bool applySlice(int index);
    void finishSlice();
    void leaveDomain();
    void saveLoop();
    void restoreLoop();
    void setLoop(mixxx::audio::FramePos start, mixxx::audio::FramePos end);

    const QString m_group;
    BaseTrackPlayer* m_player;
    std::unique_ptr<ControlObject> m_modeControl;
    std::array<std::unique_ptr<ControlPushButton>, ModeCount> m_modeButtons;
    std::array<std::unique_ptr<ControlPushButton>, 8> m_pads;
    std::array<std::unique_ptr<ControlPushButton>, 8> m_clear;
    std::array<std::unique_ptr<ControlObject>, 8> m_active;
    std::unique_ptr<ControlObject> m_slicerReady;
    std::unique_ptr<ControlObject> m_currentSlice;
    QHash<QString, ControlProxy*> m_controls;
    QTimer m_timer;
    Mode m_mode = Hotcue;
    std::array<bool, 8> m_held{};
    std::vector<int> m_sliceStack;
    mixxx::BeatsPointer m_beats;
    mixxx::audio::FramePos m_domainStart = mixxx::audio::kInvalidFramePos;
    mixxx::audio::FramePos m_domainEnd = mixxx::audio::kInvalidFramePos;
    bool m_domainLoop = false;
    bool m_slicing = false;
    bool m_previousSlip = false;
    bool m_rollSlip = false;
    double m_previousLoopStart = -1;
    double m_previousLoopEnd = -1;
    bool m_previousLoopEnabled = false;
};
