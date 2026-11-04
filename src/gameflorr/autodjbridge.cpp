#include "gameflorr/autodjbridge.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <algorithm>
#include <cmath>

#include "control/pollingcontrolproxy.h"
#include "gameflorr/livemashupcontroller.h"
#include "gameflorr/transitionplan.h"
#include "library/autodj/autodjprocessor.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "mixer/basetrackplayer.h"
#include "mixer/playermanager.h"
#include "mixer/previewdeck.h"
#include "preferences/gameflorrsettings.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "track/trackref.h"

namespace gameflorr {
namespace {
PollingControlProxy control(int deck, const QString& key) {
    return PollingControlProxy(QStringLiteral("[Channel%1]").arg(deck + 1),
            key,
            ControlFlag::AllowMissingOrInvalid);
}
bool playing(PlayerManagerInterface* players, int i) {
    return players->getDeckBase(i) &&
            control(i, QStringLiteral("play")).toBool();
}
} // namespace
AutoDjBridge::AutoDjBridge(QObject* parent,
        Library* library,
        PlayerManagerInterface* players,
        AutoDJProcessor* processor,
        int playlistId)
        : QObject(parent),
          m_library(library),
          m_players(players),
          m_processor(processor),
          m_playlistId(playlistId),
          m_folder(QFileInfo(gameflorrStatePath()).absolutePath()),
          m_analysis(library->createTrackAnalysisScheduler(1,
                  static_cast<AnalyzerModeFlags>(AnalyzerModeFlags::All |
                          AnalyzerModeFlags::LowPriority))),
          m_liveMashup(std::make_unique<LiveMashupController>(this, players, processor)) {
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(); });
    m_timer.start(500);
}
AutoDjBridge::~AutoDjBridge() {
    m_timer.stop();
}
QString AutoDjBridge::externalId(const TrackPointer& track) const {
    return track ? m_idsByPath.value(
                           QFileInfo(track->getLocation()).canonicalFilePath())
                 : QString();
}
void AutoDjBridge::relinquish(bool stopAudio) {
    if (m_owns) {
        PollingControlProxy crossfader(QStringLiteral("[Master]"),
                QStringLiteral("crossfader"),
                ControlFlag::AllowMissingOrInvalid);
        const double position = crossfader.get();
        m_processor->toggleAutoDJ(false);
        crossfader.set(position);
        m_processor->setCreativeMixing(false, 0);
    }
    if (stopAudio)
        for (int i = 0; i < m_players->numberOfDecks(); ++i) {
            if (externalId(m_players->getDeckBase(i)->getLoadedTrack())
                            .isEmpty())
                continue;
            control(i, QStringLiteral("play")).set(0);
        }
    m_owns = false;
    m_pendingStart = false;
    m_pendingNext = false;
}
void AutoDjBridge::tick() {
    QFile file(
            QDir(m_folder).filePath(QStringLiteral("mixxx-autodj-plan.json")));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 2 * 1024 * 1024) {
        if (m_owns || m_pendingStart) {
            relinquish();
            m_takeover = true;
        }
        writeState({}, m_error);
        return;
    }
    const auto plan = QJsonDocument::fromJson(file.readAll()).object();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 timestamp =
            plan.value(QStringLiteral("updatedAt")).toVariant().toLongLong();
    const QString session = plan.value(QStringLiteral("sessionId")).toString();
    if (plan.value(QStringLiteral("version")).toInt() != 1 ||
            session.isEmpty() || session.size() > 128 ||
            timestamp > now + 2000 || now - timestamp > 5000 ||
            !plan.value(QStringLiteral("online")).toBool()) {
        if (m_owns || m_pendingStart) {
            relinquish();
            m_takeover = true;
        }
        writeState({}, m_error);
        return;
    }
    if (session != m_session) {
        relinquish();
        m_session = session;
        m_processed.clear();
        m_ack = {};
        m_error.clear();
        m_takeover = false;
        m_idsByPath.clear();
        m_tracks.clear();
        m_managedQueue.clear();
    }
    if (m_owns && m_processor->getState() == AutoDJProcessor::ADJ_DISABLED &&
            !m_pendingStart) {
        relinquish();
        m_takeover = true;
    }
    QString error = m_error;
    const auto command = plan.value(QStringLiteral("control")).toObject();
    const QString id = command.value(QStringLiteral("id")).toString();
    const QString action = command.value(QStringLiteral("action")).toString();
    if (!id.isEmpty() && id.size() <= 128 && !m_processed.contains(id)) {
        m_processed.insert(id);
        if (m_processed.size() > 2048)
            m_processed = {id};
        bool ok = true;
        error.clear();
        if (action.startsWith(QStringLiteral("live-mashup-"))) {
            const auto context = command.value(QStringLiteral("context")).toString();
            const auto expires = command.value(QStringLiteral("expiresAt")).toVariant().toLongLong();
            const bool fresh = !context.isEmpty() && expires > now && expires <= now + 10000;
            ok = fresh && m_liveMashup->command(action, context);
            if (!ok) {
                error = m_liveMashup->state().value(QStringLiteral("reason")).toString();
                if (m_liveMashup->state().value(QStringLiteral("ready")).toBool()) {
                    error = tr("Commande mashup expirée ou platines modifiées. Vérifiez les stems et réessayez.");
                }
            }
        } else if (action == QStringLiteral("start") ||
                action == QStringLiteral("next")) {
            bool unsafe = m_liveMashup->busy() ||
                    m_processor->getState() != AutoDJProcessor::ADJ_DISABLED &&
                            !m_owns;
            int playingCount = 0;
            for (int i = 0; i < m_players->numberOfDecks(); ++i) {
                if (playing(m_players, i))
                    ++playingCount;
                if (control(i, QStringLiteral("passthrough")).toBool())
                    unsafe = true;
            }
            if (playingCount > 1 || unsafe) {
                ok = false;
                error =
                        tr("Arrêtez les autres platines et le passthrough "
                           "avant de lancer le conducteur.");
            } else if (action == QStringLiteral("next") && m_owns) {
                m_processor->fadeNow();
            } else {
                m_pendingStart = true;
                m_pendingNext = action == QStringLiteral("next") ||
                        (plan.value(QStringLiteral("mode")).toString() ==
                                        QStringLiteral("assisted") &&
                                playingCount == 1);
                m_takeover = false;
            }
        } else if (action == QStringLiteral("preview") || action == QStringLiteral("stop-preview")) {
            const QString group = QStringLiteral("[PreviewDeck1]");
            if (m_players->numberOfPreviewDecks() < 1 || !m_players->getPreviewDeck(0)) {
                ok = false;
                error = tr("Activez une platine de préécoute dans Mixxx.");
            } else if (action == QStringLiteral("stop-preview")) {
                PollingControlProxy(group, QStringLiteral("play"), ControlFlag::AllowMissingOrInvalid).set(0);
            } else {
                const QFileInfo info(command.value(QStringLiteral("path")).toString());
                if (!info.isAbsolute() || !info.isFile() || !info.isReadable() ||
                        !SoundSourceProxy::isFileNameSupported(info.fileName())) {
                    ok = false;
                    error = tr("Fichier de préécoute indisponible.");
                } else {
                    const auto track = m_library->trackCollectionManager()->getOrAddTrack(TrackRef::fromFilePath(info.canonicalFilePath()));
                    if (!track) {
                        ok = false;
                        error = tr("Impossible de charger la préécoute.");
                    } else {
                        PollingControlProxy(group, QStringLiteral("master"), ControlFlag::AllowMissingOrInvalid).set(0);
                        PollingControlProxy(group, QStringLiteral("pfl"), ControlFlag::AllowMissingOrInvalid).set(1);
                        m_players->getPreviewDeck(0)->slotLoadTrack(track,
#ifdef __STEM__
                                mixxx::StemChannelSelection(),
#endif
                                true);
                    }
                }
            }
        } else if (action == QStringLiteral("takeover") ||
                action == QStringLiteral("pause") ||
                action == QStringLiteral("release")) {
            relinquish(action == QStringLiteral("release"));
            m_takeover = true;
        } else if (action == QStringLiteral("seek") ||
                action == QStringLiteral("volume")) {
            for (int i = 0; i < m_players->numberOfDecks(); ++i) {
                const auto track = m_players->getDeckBase(i)->getLoadedTrack();
                if (!playing(m_players, i) || externalId(track).isEmpty())
                    continue;
                const double value =
                        command.value(QStringLiteral("value")).toDouble(-1);
                if (value < 0 || !isFinite(value)) {
                    ok = false;
                    break;
                }
                if (action == QStringLiteral("seek") &&
                        track->getDuration() > 0)
                    control(i, QStringLiteral("playposition"))
                            .set(std::clamp(
                                    value / (track->getDuration() * 1000),
                                    0.0,
                                    1.0));
                else if (action == QStringLiteral("volume"))
                    control(i, QStringLiteral("volume"))
                            .set(std::clamp(value / 100, 0.0, 1.0));
            }
        } else {
            ok = false;
            error = tr("Commande AutoDJ inconnue.");
        }
        m_ack = {{QStringLiteral("id"), id},
                {QStringLiteral("ok"), ok},
                {QStringLiteral("error"), error}};
    }
    if (!plan.value(QStringLiteral("enabled")).toBool() &&
            (m_owns || m_pendingStart))
        relinquish();
    m_assisted = plan.value(QStringLiteral("mode")).toString() ==
            QStringLiteral("assisted");
    if (m_pendingStart && m_liveMashup->busy()) {
        relinquish();
        m_takeover = true;
        error = tr("Terminez le mashup live avant de lancer AutoDJ.");
    }
    if (m_owns || m_pendingStart) {
        auto& dao = m_library->trackCollectionManager()
                            ->internalCollection()
                            ->getPlaylistDAO();
        const auto queue = plan.value(QStringLiteral("queue")).toArray();
        QList<TrackId> ready;
        for (const auto& value : queue) {
            if (ready.size() >= 5)
                break;
            const auto entry = value.toObject();
            const QString external =
                    entry.value(QStringLiteral("id")).toString();
            const QFileInfo info(
                    entry.value(QStringLiteral("path")).toString());
            if (external.isEmpty() || external.size() > 512 ||
                    !info.isAbsolute() || !info.isFile() ||
                    !info.isReadable() || info.size() <= 0 ||
                    !SoundSourceProxy::isFileNameSupported(info.fileName()))
                continue;
            const QString path = info.canonicalFilePath();
            TrackPointer track = m_tracks.value(path);
            if (!track) {
                track = m_library->trackCollectionManager()->getOrAddTrack(
                        TrackRef::fromFilePath(path));
                if (!track)
                    continue;
                if (!entry.value(QStringLiteral("title")).toString().isEmpty())
                    track->setTitle(entry.value(QStringLiteral("title")).toString());
                track->setArtist(entry.value(QStringLiteral("artist")).toString());
                m_tracks.insert(path, track);
                m_analysis->scheduleTrack(
                        AnalyzerScheduledTrack(track->getId()));
                m_analysis->resume();
            }
            m_idsByPath.insert(path, external);
            bool onAir = false;
            for (int i = 0; i < m_players->numberOfDecks(); ++i)
                if (playing(m_players, i) &&
                        m_players->getDeckBase(i)->getLoadedTrack() == track)
                    onAir = true;
            if (!onAir && !ready.contains(track->getId()))
                ready.append(track->getId());
        }
        const auto state = m_processor->getState();
        if (state == AutoDJProcessor::ADJ_DISABLED ||
                state == AutoDJProcessor::ADJ_IDLE) {
            auto existing = dao.getAutoDJTrackIds();
            int insertAt = 0;
            if (m_owns && !existing.isEmpty()) {
                const auto first =
                        m_library->trackCollectionManager()->getTrackById(
                                existing.first());
                bool preloaded = false;
                for (int i = 0; i < m_players->numberOfDecks(); ++i)
                    if (m_players->getDeckBase(i)->getLoadedTrack() == first)
                        preloaded = true;
                if (preloaded && ready.contains(existing.first())) {
                    ready.removeAll(existing.first());
                    insertAt = 1;
                }
            }
            QList<TrackId> wanted;
            if (insertAt)
                wanted.append(existing.first());
            wanted.append(ready);
            for (const auto& track : existing)
                if (!m_managedQueue.contains(track) && !wanted.contains(track))
                    wanted.append(track);
            if (wanted != existing) {
                for (const auto& track : existing) {
                    if (ready.contains(track) ||
                            (m_managedQueue.contains(track) &&
                                    !wanted.contains(track)))
                        dao.removeTracksFromPlaylistById(m_playlistId, track);
                }
                dao.insertTracksIntoPlaylist(ready, m_playlistId, insertAt + 1);
                for (const auto& track : ready)
                    m_managedQueue.insert(track);
            }
        }
        if (state == AutoDJProcessor::ADJ_DISABLED ||
                state == AutoDJProcessor::ADJ_IDLE)
            m_processor->setCreativeMixing(
                    plan.value(QStringLiteral("mode")).toString() ==
                            QStringLiteral("creative"),
                    plan.value(QStringLiteral("creativity")).toDouble(0.4),
                    plan.value(QStringLiteral("stemsEnabled")).toBool() && plan.value(QStringLiteral("mashupEnabled")).toBool());
        if (m_pendingStart && !dao.getAutoDJTrackIds().isEmpty()) {
            m_processor->setTransitionMode(
                    AutoDJProcessor::TransitionMode::FullIntroOutro);
            m_processor->setTransitionTime(10);
            const auto result = m_processor->toggleAutoDJ(true);
            m_pendingStart = false;
            m_owns = result == AutoDJProcessor::ADJ_OK;
            if (!m_owns) {
                m_takeover = true;
                error = tr("Mixxx n’a pas pu démarrer AutoDJ (%1).")
                                .arg(result);
                m_ack.insert(QStringLiteral("ok"), false);
                m_ack.insert(QStringLiteral("error"), error);
            }
        }
        if (m_owns && m_pendingNext &&
                m_processor->getState() == AutoDJProcessor::ADJ_IDLE &&
                m_processor->nextTrackLoaded()) {
            m_processor->fadeNow();
            m_pendingNext = false;
            m_observedFade = false;
        }
        if (m_owns && m_assisted) {
            const auto currentState = m_processor->getState();
            if (currentState == AutoDJProcessor::ADJ_LEFT_FADING ||
                    currentState == AutoDJProcessor::ADJ_RIGHT_FADING)
                m_observedFade = true;
            if (currentState == AutoDJProcessor::ADJ_IDLE && !m_pendingNext &&
                    (!m_observedFade || m_processor->nextTrackLoaded())) {
                relinquish();
                m_takeover = true;
                m_observedFade = false;
            }
        }
    }
    if (m_tracks.size() > 128) {
        QSet<QString> keep;
        for (const auto& entry : plan.value(QStringLiteral("queue")).toArray())
            keep.insert(QFileInfo(
                    entry.toObject().value(QStringLiteral("path")).toString())
                            .canonicalFilePath());
        for (int i = 0; i < m_players->numberOfDecks(); ++i) {
            const auto track = m_players->getDeckBase(i)->getLoadedTrack();
            if (track)
                keep.insert(
                        QFileInfo(track->getLocation()).canonicalFilePath());
        }
        for (auto it = m_tracks.begin(); it != m_tracks.end();) {
            if (!keep.contains(it.key())) {
                m_idsByPath.remove(it.key());
                it = m_tracks.erase(it);
            } else
                ++it;
        }
    }
    m_error = error;
    writeState(plan, error);
}
void AutoDjBridge::writeState(const QJsonObject& plan, const QString& error) {
    QJsonArray metadata;
    for (auto it = m_tracks.cbegin(); it != m_tracks.cend(); ++it) {
        const auto& track = it.value();
        metadata.append(QJsonObject{{QStringLiteral("id"), externalId(track)},
                {QStringLiteral("bpm"), track->getBpm()},
                {QStringLiteral("key"), track->getKeyText()},
                {QStringLiteral("durationMs"), track->getDuration() * 1000}});
    }
    TrackPointer onAir;
    int deck = -1;
    double best = -1;
    PollingControlProxy xf(QStringLiteral("[Master]"),
            QStringLiteral("crossfader"),
            ControlFlag::AllowMissingOrInvalid);
    PollingControlProxy reverse(QStringLiteral("[Mixer Profile]"),
            QStringLiteral("xFaderReverse"),
            ControlFlag::AllowMissingOrInvalid);
    const double crossfader = reverse.toBool() ? -xf.get() : xf.get();
    for (int i = 0; i < m_players->numberOfDecks(); ++i) {
        const auto track = m_players->getDeckBase(i)->getLoadedTrack();
        if (!playing(m_players, i) || externalId(track).isEmpty())
            continue;
        const double orientation =
                control(i, QStringLiteral("orientation")).get();
        const double gain = control(i, QStringLiteral("volume")).get() *
                (orientation == 0                  ? (1 - crossfader) / 2
                                : orientation == 2 ? (1 + crossfader) / 2
                                                   : 1);
        if (gain > best && gain > 0.01) {
            best = gain;
            deck = i;
            onAir = track;
        }
    }
    QJsonObject state{{QStringLiteral("version"), 1},
            {QStringLiteral("agentSessionId"), m_session},
            {QStringLiteral("updatedAt"), QDateTime::currentMSecsSinceEpoch()},
            {QStringLiteral("enabled"), m_owns || m_pendingStart},
            {QStringLiteral("takeover"), m_takeover},
            {QStringLiteral("isPlaying"), bool(onAir)},
            {QStringLiteral("currentId"), externalId(onAir)},
            {QStringLiteral("positionMs"),
                    onAir ? control(deck, QStringLiteral("playposition"))
                                            .get() *
                                    onAir->getDuration() * 1000
                          : 0},
            {QStringLiteral("durationMs"),
                    onAir ? onAir->getDuration() * 1000 : 0},
            {QStringLiteral("metadata"), metadata},
            {QStringLiteral("liveMashup"), m_liveMashup->state()},
            {QStringLiteral("lastCommand"), m_ack},
            {QStringLiteral("error"), error},
            {QStringLiteral("transition"),
                    m_processor->creativeTransitionDescription()}};
    Q_UNUSED(plan);
    QDir().mkpath(m_folder);
    QSaveFile output(QDir(m_folder).filePath(
            QStringLiteral("gameflorr-autodj-state.json")));
    if (output.open(QIODevice::WriteOnly)) {
        output.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        output.write(QJsonDocument(state).toJson(QJsonDocument::Compact));
        output.commit();
    }
}
} // namespace gameflorr
