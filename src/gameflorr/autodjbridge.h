#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QTimer>

#include "analyzer/trackanalysisscheduler.h"
#include "track/track_decl.h"

class Library;
class AutoDJProcessor;
class PlayerManagerInterface;
namespace gameflorr {
class AutoDjBridge : public QObject {
  public:
    AutoDjBridge(QObject* parent,
            Library* library,
            PlayerManagerInterface* players,
            AutoDJProcessor* processor,
            int playlistId);
    ~AutoDjBridge() override;

  private:
    void tick();
    void relinquish(bool stopAudio = false);
    void writeState(const QJsonObject& plan, const QString& error);
    QString externalId(const TrackPointer& track) const;
    Library* m_library;
    PlayerManagerInterface* m_players;
    AutoDJProcessor* m_processor;
    int m_playlistId;
    QTimer m_timer;
    QString m_folder;
    QString m_session;
    QString m_error;
    QJsonObject m_ack;
    QHash<QString, QString> m_idsByPath;
    QHash<QString, TrackPointer> m_tracks;
    QSet<QString> m_processed;
    QSet<TrackId> m_managedQueue;
    bool m_owns = false;
    bool m_takeover = false;
    bool m_pendingStart = false;
    bool m_pendingNext = false;
    bool m_assisted = true;
    bool m_observedFade = false;
    TrackAnalysisScheduler::Pointer m_analysis;
};
} // namespace gameflorr
