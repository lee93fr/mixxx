#include "mixer/deck.h"

#include "moc_deck.cpp"
#include "mixer/performancepadcontrol.h"

Deck::Deck(PlayerManager* pParent,
        UserSettingsPointer pConfig,
        EngineMixer* pMixingEngine,
        EffectsManager* pEffectsManager,
        EngineChannel::ChannelOrientation defaultOrientation,
        const ChannelHandleAndGroup& handleGroup)
        : BaseTrackPlayerImpl(pParent,
                  pConfig,
                  pMixingEngine,
                  pEffectsManager,
                  defaultOrientation,
                  handleGroup,
                  /*defaultMainMix*/ true,
                  /*defaultHeadphones*/ false,
                  /*primaryDeck*/ true) {
    m_performancePads = std::make_unique<PerformancePadControl>(handleGroup.name(), this);
}

Deck::~Deck() = default;
