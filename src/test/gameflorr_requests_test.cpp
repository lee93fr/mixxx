#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryFile>

#include "gameflorr/requestsnapshot.h"

namespace {
QJsonObject snapshot() {
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("sessionId"), QStringLiteral("session")},
            {QStringLiteral("updatedAt"), 100000},
            {QStringLiteral("online"), true},
            {QStringLiteral("backendConnected"), true},
            {QStringLiteral("downloadsEnabled"), false},
            {QStringLiteral("requests"),
                    QJsonArray{QJsonObject{
                            {QStringLiteral("id"), QStringLiteral("request")},
                            {QStringLiteral("title"), QStringLiteral("Titre")},
                            {QStringLiteral("artist"),
                                    QStringLiteral("Artiste")},
                            {QStringLiteral("stage"),
                                    QStringLiteral("queued")}}}}};
}
auto parse(const QJsonObject& value, qint64 now = 100000) {
    return gameflorr::parseRequestSnapshot(QJsonDocument(value).toJson(), now);
}
} // namespace

TEST(GameflorrRequests, AcceptsRequestsWithoutDownloadCredentials) {
    const auto result = parse(snapshot());
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->online);
    EXPECT_TRUE(result->backendConnected);
    EXPECT_FALSE(result->downloadsEnabled);
    ASSERT_EQ(result->requests.size(), 1);
    EXPECT_EQ(result->requests[0].title, QStringLiteral("Titre"));
}

TEST(GameflorrRequests, RejectsStaleFutureInvalidAndOversizedSnapshots) {
    EXPECT_FALSE(parse(snapshot(), 105001));
    EXPECT_FALSE(parse(snapshot(), 97999));
    auto value = snapshot();
    value.insert(QStringLiteral("version"), 2);
    EXPECT_FALSE(parse(value));
    value = snapshot();
    value.insert(QStringLiteral("sessionId"), QString());
    EXPECT_FALSE(parse(value));
    EXPECT_FALSE(gameflorr::parseRequestSnapshot("not json", 100000));
    EXPECT_FALSE(gameflorr::parseRequestSnapshot(
            QByteArray(2 * 1024 * 1024 + 1, ' '), 100000));
}

TEST(GameflorrRequests, RejectsDuplicatesUnknownStagesAndMissingTitles) {
    auto value = snapshot();
    auto requests = value.value(QStringLiteral("requests")).toArray();
    requests.append(requests[0]);
    value.insert(QStringLiteral("requests"), requests);
    EXPECT_FALSE(parse(value));
    auto request = requests[0].toObject();
    request.insert(QStringLiteral("stage"), QStringLiteral("play"));
    value.insert(QStringLiteral("requests"), QJsonArray{request});
    EXPECT_FALSE(parse(value));
    request.insert(QStringLiteral("stage"), QStringLiteral("ready"));
    request.remove(QStringLiteral("title"));
    value.insert(QStringLiteral("requests"), QJsonArray{request});
    EXPECT_FALSE(parse(value));
}

TEST(GameflorrRequests,
        LoadRequiresReadableFileAndStoppedDeckWithoutPassthrough) {
    QTemporaryFile file;
    ASSERT_TRUE(file.open());
    gameflorr::Request request{QStringLiteral("id"),
            QStringLiteral("Titre"),
            QStringLiteral("Artiste"),
            QStringLiteral("ready"),
            file.fileName(),
            {}};
    EXPECT_TRUE(gameflorr::canLoadRequest(request, false, false));
    EXPECT_FALSE(gameflorr::canLoadRequest(request, true, false));
    EXPECT_FALSE(gameflorr::canLoadRequest(request, false, true));
    request.stage = QStringLiteral("downloading");
    EXPECT_FALSE(gameflorr::canLoadRequest(request, false, false));
    request.stage = QStringLiteral("skipped");
    EXPECT_TRUE(gameflorr::canLoadRequest(request, false, false));
    request.filePath = QStringLiteral("relative.m4a");
    EXPECT_FALSE(gameflorr::canLoadRequest(request, false, false));
    request.filePath = QStringLiteral("/nonexistent/gameflorr-request.m4a");
    EXPECT_FALSE(gameflorr::canLoadRequest(request, false, false));
}
