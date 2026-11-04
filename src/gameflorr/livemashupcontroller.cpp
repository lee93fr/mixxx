#include "gameflorr/livemashupcontroller.h"

#include <QUuid>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "library/autodj/autodjprocessor.h"
#include "mixer/basetrackplayer.h"
#include "mixer/playermanager.h"
#include "track/beats.h"
#include "track/track.h"

namespace gameflorr {
namespace {
PollingControlProxy deckControl(int deck, const QString& key) {
    return PollingControlProxy(QStringLiteral("[Channel%1]").arg(deck + 1), key, ControlFlag::AllowMissingOrInvalid);
}
bool canonical(const TrackPointer& track) {
#ifdef __STEM__
    const auto info = track->getStemInfo();
    const QStringList expected = {QStringLiteral("Drums"), QStringLiteral("Bass"), QStringLiteral("Other"), QStringLiteral("Vocals")};
    if (info.size() != expected.size()) {
        return false;
    }
    for (int i = 0; i < info.size(); ++i) {
        if (info[i].getLabel() != expected[i]) {
            return false;
        }
    }
    return true;
#else
    static_cast<void>(track);
    return false;
#endif
}
} // namespace

QString liveMashupReason(int status) {
    switch (status) {
    case 0:
        return QObject::tr("Prêt · départs, volumes et crossfader sous votre contrôle.");
    case 1:
        return QObject::tr("Reprenez la main sur AutoDJ avant le mashup live.");
    case 2:
        return QObject::tr("Chargez deux titres avec les 4 stems préparés sur A et B.");
    case 3:
        return QObject::tr("Démarrez les deux platines et désactivez le passthrough.");
    case 4:
        return QObject::tr("Grilles stables, même tonalité connue et verrouillage de tonalité requis.");
    case 5:
        return QObject::tr("Synchronisez les tempos de A et B (écart maximal 0,5 %).");
    case 6:
        return QObject::tr("Rétablissez les stems sans mute, avec des gains entre 0 et 1.");
    default:
        return QObject::tr("Retour au mix complet en cours.");
    }
}

LiveMashupController::LiveMashupController(QObject* parent, PlayerManagerInterface* players, AutoDJProcessor* processor)
        : QObject(parent),
          m_players(players),
          m_processor(processor),
          m_mode(std::make_unique<ControlObject>(ConfigKey(QStringLiteral("[Gameflorr]"), QStringLiteral("live_mashup_mode")))),
          m_status(std::make_unique<ControlObject>(ConfigKey(QStringLiteral("[Gameflorr]"), QStringLiteral("live_mashup_status")))) {
    m_mode->setReadOnly();
    m_status->setReadOnly();
    const QStringList actions = {QStringLiteral("live-mashup-a"), QStringLiteral("live-mashup-b"), QStringLiteral("live-mashup-swap"), QStringLiteral("live-mashup-reset")};
    const QStringList keys = {QStringLiteral("live_mashup_a"), QStringLiteral("live_mashup_b"), QStringLiteral("live_mashup_swap"), QStringLiteral("live_mashup_reset")};
    for (int i = 0; i < 4; ++i) {
        m_buttons[i] = std::make_unique<ControlPushButton>(ConfigKey(QStringLiteral("[Gameflorr]"), keys[i]));
        connect(m_buttons[i].get(), &ControlObject::valueChanged, this, [this, action = actions[i]](double value) {
            if (value > 0) {
                command(action);
            }
        });
    }
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(); });
    m_elapsed.start();
    m_timer.start(20);
    tick();
}

LiveMashupController::~LiveMashupController() {
    m_timer.stop();
    protectLoadedTracks();
    m_mashup.restore();
}

int LiveMashupController::availability() const {
    if (m_processor->getState() != AutoDJProcessor::ADJ_DISABLED) {
        return 1;
    }
    if (m_players->numberOfDecks() < 2) {
        return 2;
    }
    std::array<TrackPointer, 2> tracks;
    std::array<double, 2> tempos;
    for (int i = 0; i < 2; ++i) {
        const auto* deck = m_players->getDeckBase(i);
        tracks[i] = deck ? deck->getLoadedTrack() : TrackPointer();
        if (!tracks[i] || !canonical(tracks[i])) {
            return 2;
        }
        if (!deckControl(i, QStringLiteral("play")).toBool() || deckControl(i, QStringLiteral("passthrough")).toBool()) {
            return 3;
        }
        if (!tracks[i]->getBeats() || !tracks[i]->getBeats()->hasConstantTempo() || tracks[i]->getKey() == mixxx::track::io::key::INVALID || !deckControl(i, QStringLiteral("keylock")).toBool()) {
            return 4;
        }
        tempos[i] = tracks[i]->getBpm() * deckControl(i, QStringLiteral("rate_ratio")).get();
    }
    if (tracks[0]->getKey() != tracks[1]->getKey()) {
        return 4;
    }
    if (!liveMashupTempoCompatible(tempos[0], tempos[1])) {
        return 5;
    }
    if (!m_mashup.busy()) {
        for (int i = 0; i < 8; ++i) {
            const QString group = QStringLiteral("[Channel%1_Stem%2]").arg(i / 4 + 1).arg(i % 4 + 1);
            PollingControlProxy volume(group, QStringLiteral("volume"), ControlFlag::AllowMissingOrInvalid);
            PollingControlProxy mute(group, QStringLiteral("mute"), ControlFlag::AllowMissingOrInvalid);
            if (!volume.valid() || !mute.valid() || mute.toBool() || !isFinite(volume.get()) || volume.get() < 0 || volume.get() > 1) {
                return 6;
            }
        }
    }
    return 0;
}

void LiveMashupController::protectLoadedTracks() {
    for (int i = 0; i < 2; ++i) {
        const auto* deck = i < m_players->numberOfDecks() ? m_players->getDeckBase(i) : nullptr;
        const auto track = deck ? deck->getLoadedTrack() : TrackPointer();
        if (m_context.isEmpty() || track != m_observedTracks[i]) {
            m_observedTracks[i] = track;
            m_context = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        if (m_mashup.busy() && track != m_tracks[i]) {
            m_mashup.abandonDeck(i);
            m_mashup.stop();
        }
    }
}

bool LiveMashupController::command(const QString& action, const QString& context) {
    protectLoadedTracks();
    if (!context.isEmpty() && context != m_context) {
        return false;
    }
    if (action == QStringLiteral("live-mashup-reset")) {
        m_mashup.stop();
        return true;
    }
    int mode = action == QStringLiteral("live-mashup-a") ? 1 : action == QStringLiteral("live-mashup-b") ? 2
                                                                                                         : 0;
    if (action == QStringLiteral("live-mashup-swap") && m_mashup.mode()) {
        mode = 3 - m_mashup.mode();
    }
    if (!mode || availability() || (m_mashup.busy() && !m_mashup.mode())) {
        return false;
    }
    const bool wasBusy = m_mashup.busy();
    if (!m_mashup.select(mode)) {
        m_status->setAndConfirm(6);
        return false;
    }
    if (!wasBusy) {
        for (int i = 0; i < 2; ++i) {
            m_tracks[i] = m_players->getDeckBase(i)->getLoadedTrack();
        }
    }
    m_elapsed.restart();
    m_mode->setAndConfirm(mode);
    return true;
}

void LiveMashupController::tick() {
    protectLoadedTracks();
    const int status = availability();
    if (status && m_mashup.mode()) {
        m_mashup.stop();
    }
    m_mashup.advance(m_elapsed.restart() / 1000.0);
    m_mode->setAndConfirm(m_mashup.mode());
    m_status->setAndConfirm(m_mashup.busy() && !m_mashup.mode() ? 7 : status);
}

QJsonObject LiveMashupController::state() const {
    const int status = static_cast<int>(m_status->get());
    return {{QStringLiteral("mode"), m_mashup.mode()},
            {QStringLiteral("context"), m_context},
            {QStringLiteral("busy"), m_mashup.busy()},
            {QStringLiteral("ready"), status == 0},
            {QStringLiteral("reason"), liveMashupReason(status)}};
}
} // namespace gameflorr
