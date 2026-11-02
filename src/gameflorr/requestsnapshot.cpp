#include "gameflorr/requestsnapshot.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

#include "preferences/gameflorrsettings.h"

namespace gameflorr {
std::optional<RequestSnapshot> parseRequestSnapshot(
        const QByteArray& data, qint64 now) {
    if (data.size() > 2 * 1024 * 1024) {
        return std::nullopt;
    }
    const auto document = QJsonDocument::fromJson(data);
    if (!document.isObject()) {
        return std::nullopt;
    }
    const auto root = document.object();
    const auto updatedAt = root.value(QStringLiteral("updatedAt")).toDouble(0);
    if (root.value(QStringLiteral("version")).toInt() != 1 ||
            root.value(QStringLiteral("sessionId")).toString().isEmpty() ||
            updatedAt <= 0 || updatedAt > now + 2000 ||
            now - updatedAt > 5000 ||
            !root.value(QStringLiteral("requests")).isArray()) {
        return std::nullopt;
    }
    RequestSnapshot result{root.value(QStringLiteral("sessionId")).toString(),
            root.value(QStringLiteral("online")).toBool(),
            root.value(QStringLiteral("backendConnected")).toBool(),
            root.value(QStringLiteral("downloadsEnabled")).toBool(),
            {},
            root.value(QStringLiteral("lastCommand")).toObject()};
    const auto requests = root.value(QStringLiteral("requests")).toArray();
    if (requests.size() > 500) {
        return std::nullopt;
    }
    const QSet<QString> stages{QStringLiteral("queued"),
            QStringLiteral("searching"),
            QStringLiteral("downloading"),
            QStringLiteral("ready"),
            QStringLiteral("skipped"),
            QStringLiteral("error")};
    QSet<QString> ids;
    for (const auto& value : requests) {
        const auto item = value.toObject();
        Request request{item.value(QStringLiteral("id")).toString(),
                item.value(QStringLiteral("title")).toString(),
                item.value(QStringLiteral("artist")).toString(),
                item.value(QStringLiteral("stage")).toString(),
                item.value(QStringLiteral("filePath")).toString(),
                item.value(QStringLiteral("error")).toString()};
        if (request.id.isEmpty() || request.id.size() > 512 ||
                ids.contains(request.id) || request.title.isEmpty() ||
                !stages.contains(request.stage)) {
            return std::nullopt;
        }
        ids.insert(request.id);
        result.requests.append(std::move(request));
    }
    return result;
}

QString requestsPath() {
    return QFileInfo(gameflorrStatePath())
            .absoluteDir()
            .filePath(QStringLiteral("gameflorr-requests.json"));
}

QString commandPath() {
    return QFileInfo(gameflorrStatePath())
            .absoluteDir()
            .filePath(QStringLiteral("gameflorr-command.json"));
}

bool canLoadRequest(const Request& request, bool playing, bool passthrough) {
    const QFileInfo file(request.filePath);
    return !playing && !passthrough &&
            (request.stage == QStringLiteral("ready") ||
                    request.stage == QStringLiteral("skipped")) &&
            file.isAbsolute() && file.isFile() && file.isReadable();
}
} // namespace gameflorr
