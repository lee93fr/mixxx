#pragma once

#include <QJsonObject>
#include <QSet>
#include <QWidget>
#include <functional>
#include <optional>

#include "gameflorr/requestsnapshot.h"
#include "preferences/usersettings.h"

class ControllerManager;
class Library;
class SoundManager;
class QLabel;
class QDialog;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QWizard;

namespace gameflorr {
QWidget* createAutoDjPanel(QWidget* parent);
class MusicdlPanel : public QWidget {
  public:
    MusicdlPanel(QWidget* parent, Library* library);

  private:
    void refresh();
    void updateActions();
    QJsonObject selectedPlaylist() const;
    void sendCommand(const QString& action);
    Library* m_pLibrary;
    QLabel* m_pStatus;
    QLabel* m_pFeedback;
    QLineEdit* m_pFilter;
    QTableWidget* m_pTable;
    QPushButton* m_pRefresh;
    QPushButton* m_pPrepare;
    QPushButton* m_pImport;
    QJsonObject m_snapshot;
    QString m_pendingCommand;
    qint64 m_commandSentAt = 0;
    QSet<QString> m_importedPaths;
};

QDialog* createSettingsDialog(QWidget* parent, UserSettingsPointer settings,
        std::function<void(bool)> setScreenButtonVisible);
QDialog* createDisplayDialog(QWidget* parent);

QWizard* createSetupWizard(QWidget* parent,
        UserSettingsPointer settings,
        SoundManager* soundManager,
        ControllerManager* controllerManager,
        std::function<void(int)> openPreferences);

class RequestsPanel : public QWidget {
  public:
    RequestsPanel(QWidget* parent, Library* library);

  private:
    void refresh();
    void updateActions();
    const Request* selectedRequest() const;
    void load(int deck);
    void download();

    Library* m_pLibrary;
    QLabel* m_pStatus;
    QLabel* m_pFeedback;
    QLineEdit* m_pFilter;
    QTableWidget* m_pTable;
    QPushButton* m_pDownload;
    QPushButton* m_pLoad1;
    QPushButton* m_pLoad2;
    std::optional<RequestSnapshot> m_snapshot;
    QByteArray m_lastData;
    QString m_pendingCommand;
    qint64 m_commandSentAt = 0;
};
} // namespace gameflorr
