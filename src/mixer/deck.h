#pragma once

#include <QObject>

#include "mixer/basetrackplayer.h"

class PerformancePadControl;

class Deck : public BaseTrackPlayerImpl {
    Q_OBJECT
  public:
    Deck(PlayerManager* pParent,
            UserSettingsPointer pConfig,
            EngineMixer* pMixingEngine,
            EffectsManager* pEffectsManager,
            EngineChannel::ChannelOrientation defaultOrientation,
            const ChannelHandleAndGroup& handleGroup);
    ~Deck() override;

  private:
    std::unique_ptr<PerformancePadControl> m_performancePads;
};
