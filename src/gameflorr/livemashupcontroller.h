#pragma once

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <memory>

#include "gameflorr/livemashup.h"
#include "track/track_decl.h"

class ControlObject;
class ControlPushButton;
class AutoDJProcessor;
class PlayerManagerInterface;

namespace gameflorr {
QString liveMashupReason(int status);

class LiveMashupController : public QObject {
  public:
    LiveMashupController(QObject* parent, PlayerManagerInterface* players, AutoDJProcessor* processor);
    ~LiveMashupController() override;
    bool command(const QString& action, const QString& context = {});
    bool busy() const {
        return m_mashup.busy();
    }
    QJsonObject state() const;

  private:
    int availability() const;
    void tick();
    void protectLoadedTracks();
    void shutdown();
    QPointer<PlayerManagerInterface> m_players;
    QPointer<AutoDJProcessor> m_processor;
    bool m_shuttingDown = false;
    LiveMashup m_mashup;
    std::array<TrackPointer, 2> m_tracks;
    std::array<TrackPointer, 2> m_observedTracks;
    QString m_context;
    std::array<std::unique_ptr<ControlPushButton>, 4> m_buttons;
    std::unique_ptr<ControlObject> m_mode;
    std::unique_ptr<ControlObject> m_status;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
};
} // namespace gameflorr
