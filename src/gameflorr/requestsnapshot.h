#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>
#include <optional>

namespace gameflorr {
struct Request {
    QString id;
    QString title;
    QString artist;
    QString stage;
    QString filePath;
    QString error;
};

struct RequestSnapshot {
    QString sessionId;
    bool online;
    bool backendConnected;
    bool downloadsEnabled;
    QVector<Request> requests;
    QJsonObject lastCommand;
};

std::optional<RequestSnapshot> parseRequestSnapshot(
        const QByteArray& data, qint64 now);
QString requestsPath();
QString commandPath();
bool canLoadRequest(const Request& request, bool playing, bool passthrough);
} // namespace gameflorr
