#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSlider>
#include <QTableWidget>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

#include "control/pollingcontrolproxy.h"
#include "gameflorr/gameflorrpanel.h"
#include "gameflorr/livemashupcontroller.h"
#include "preferences/gameflorrsettings.h"

namespace gameflorr {
QWidget* createAutoDjPanel(QWidget* parent) {
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    auto* panel = new QWidget(scroll);
    scroll->setWidget(panel);
    auto* layout = new QVBoxLayout(panel);
    auto* liveBox = new QGroupBox(QObject::tr("Mashup live · platines A et B"), panel);
    auto* liveLayout = new QVBoxLayout(liveBox);
    auto* liveStatus = new QLabel(liveBox);
    liveStatus->setWordWrap(true);
    liveStatus->setTextFormat(Qt::PlainText);
    liveLayout->addWidget(liveStatus);
    auto* liveActions = new QHBoxLayout();
    liveLayout->addLayout(liveActions);
    auto liveButtons = std::make_shared<QList<QPushButton*>>();
    const QStringList liveLabels = {QObject::tr("Voix A + instru B"), QObject::tr("Voix B + instru A"), QObject::tr("Passer la voix"), QObject::tr("Mix complet")};
    const QStringList liveKeys = {QStringLiteral("live_mashup_a"), QStringLiteral("live_mashup_b"), QStringLiteral("live_mashup_swap"), QStringLiteral("live_mashup_reset")};
    for (int i = 0; i < liveLabels.size(); ++i) {
        auto* button = new QPushButton(liveLabels[i], liveBox);
        button->setCheckable(i < 2);
        liveButtons->append(button);
        liveActions->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, panel, [key = liveKeys[i]] {
            PollingControlProxy control(QStringLiteral("[Gameflorr]"), key, ControlFlag::AllowMissingOrInvalid);
            control.set(1);
            control.set(0);
        });
    }
    layout->addWidget(liveBox);
    auto* status = new QLabel(panel);
    status->setWordWrap(true);
    status->setTextFormat(Qt::PlainText);
    layout->addWidget(status);
    auto* form = new QFormLayout();
    layout->addLayout(form);
    auto* mode = new QComboBox(panel);
    mode->addItem(
            QObject::tr("Assisté — autoriser chaque enchaînement"), "assisted");
    mode->addItem(QObject::tr("Automatique sobre"), "gentle");
    mode->addItem(QObject::tr("Automatique créatif"), "creative");
    form->addRow(QObject::tr("Mix"), mode);
    auto* theme = new QLineEdit(panel);
    theme->setPlaceholderText(QObject::tr("Ex. disco, années 90…"));
    form->addRow(QObject::tr("Thème"), theme);
    auto* energy = new QComboBox(panel);
    const QStringList energies = {
            "calme", "repas", "apero", "montee", "dancefloor", "pic", "retour"};
    energy->addItems(energies);
    form->addRow(QObject::tr("Énergie"), energy);
    auto* intensity = new QSlider(Qt::Horizontal, panel);
    intensity->setRange(0, 100);
    intensity->setValue(40);
    form->addRow(QObject::tr("Créativité"), intensity);
    auto* weight = new QSlider(Qt::Horizontal, panel);
    weight->setRange(0, 100);
    weight->setValue(40);
    form->addRow(QObject::tr("Place des demandes"), weight);
    auto* stems = new QCheckBox(QObject::tr("Utiliser les 4 stems préparés"), panel);
    auto* mashup = new QCheckBox(QObject::tr("Mashup vocal en mode créatif (même tonalité)"), panel);
    form->addRow(stems);
    form->addRow(mashup);
    auto* excluded = new QLineEdit(panel);
    excluded->setPlaceholderText(
            QObject::tr("Artistes séparés par une virgule"));
    form->addRow(QObject::tr("Artistes exclus"), excluded);
    auto* playlists = new QListWidget(panel);
    playlists->setMaximumHeight(90);
    form->addRow(QObject::tr("Playlists MusicDL"), playlists);
    auto* schedule = new QTableWidget(0, 7, panel);
    schedule->setHorizontalHeaderLabels({QObject::tr("Début ISO + fuseau"),
            QObject::tr("Fin ISO + fuseau"),
            QObject::tr("Thème"),
            QObject::tr("Énergie"),
            QObject::tr("IDs playlists (virgules)"),
            QObject::tr("ID titre imposé"),
            "ID"});
    schedule->hideColumn(6);
    schedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    schedule->setMaximumHeight(130);
    layout->addWidget(schedule);
    auto* scheduleButtons = new QHBoxLayout();
    layout->addLayout(scheduleButtons);
    auto* add = new QPushButton(QObject::tr("Ajouter un créneau"), panel);
    auto* remove = new QPushButton(QObject::tr("Retirer le créneau"), panel);
    scheduleButtons->addWidget(add);
    scheduleButtons->addWidget(remove);
    scheduleButtons->addStretch();
    QObject::connect(add, &QPushButton::clicked, panel, [schedule] {
        const int row = schedule->rowCount();
        schedule->insertRow(row);
        const auto previousEnd = row > 0 && schedule->item(row - 1, 1)
                ? QDateTime::fromString(
                          schedule->item(row - 1, 1)->text(), Qt::ISODate)
                : QDateTime();
        const auto now = previousEnd.isValid()
                ? previousEnd
                : QDateTime::currentDateTimeUtc();
        const QStringList values = {now.toString(Qt::ISODate),
                now.addSecs(3600).toString(Qt::ISODate),
                "",
                "apero",
                "",
                "",
                QUuid::createUuid().toString(QUuid::WithoutBraces)};
        for (int c = 0; c < values.size(); ++c)
            schedule->setItem(row, c, new QTableWidgetItem(values[c]));
    });
    QObject::connect(remove, &QPushButton::clicked, panel, [schedule] {
        if (schedule->currentRow() >= 0)
            schedule->removeRow(schedule->currentRow());
    });
    auto* preview = new QTableWidget(0, 4, panel);
    preview->setHorizontalHeaderLabels({QObject::tr("Titre / artiste"),
            QObject::tr("Préparation"),
            QObject::tr("Pourquoi"),
            "ID"});
    preview->setMinimumHeight(150);
    preview->setMaximumHeight(230);
    preview->hideColumn(3);
    preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    preview->setSelectionBehavior(QAbstractItemView::SelectRows);
    preview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(preview);
    auto* feedback = new QLabel(panel);
    feedback->setTextFormat(Qt::PlainText);
    feedback->setWordWrap(true);
    layout->addWidget(feedback);
    auto* actions = new QHBoxLayout();
    layout->addLayout(actions);
    auto* manualActions = new QHBoxLayout();
    layout->addLayout(manualActions);
    auto* apply = new QPushButton(QObject::tr("Appliquer le programme"), panel);
    auto* start = new QPushButton(QObject::tr("Lancer / autoriser"), panel);
    auto* next = new QPushButton(QObject::tr("Enchaîner maintenant"), panel);
    auto* lock = new QPushButton(
            QObject::tr("Verrouiller / libérer le titre"), panel);
    auto* prepare = new QPushButton(QObject::tr("Préparer 4 stems"), panel);
    auto* cancel = new QPushButton(QObject::tr("Annuler préparation"), panel);
    auto* listen = new QPushButton(QObject::tr("Préécouter au casque"), panel);
    auto* stopListen = new QPushButton(QObject::tr("Arrêter préécoute"), panel);
    auto* stemActions = new QHBoxLayout();
    layout->addLayout(stemActions);
    for (auto* button : {prepare, cancel, listen, stopListen})
        stemActions->addWidget(button);
    auto* takeover = new QPushButton(QObject::tr("Reprendre la main"), panel);
    for (auto* button : {apply, start, next})
        actions->addWidget(button);
    for (auto* button : {lock, takeover})
        manualActions->addWidget(button);
    actions->setSpacing(4);
    auto state = std::make_shared<QJsonObject>();
    auto initialized = std::make_shared<QString>();
    auto send = [panel, state, feedback](
                        const QString& action, QJsonObject extra = {}) {
        const auto session = state->value("sessionId").toString();
        const auto timestamp =
                state->value("updatedAt").toVariant().toLongLong();
        if (session.isEmpty() ||
                timestamp > QDateTime::currentMSecsSinceEpoch() + 2000 ||
                QDateTime::currentMSecsSinceEpoch() - timestamp > 5000) {
            feedback->setText(QObject::tr(
                    "Connectez l’agent Gameflorr avec la source Mixxx."));
            return;
        }
        extra.insert("version", 1);
        extra.insert("sessionId", session);
        extra.insert("updatedAt", QDateTime::currentMSecsSinceEpoch());
        extra.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
        extra.insert("action", action);
        QSaveFile file(QFileInfo(gameflorrStatePath())
                        .absoluteDir()
                        .filePath("gameflorr-autodj-command.json"));
        if (!file.open(QIODevice::WriteOnly)) {
            feedback->setText(QObject::tr("Impossible d’écrire la commande."));
            return;
        }
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        file.write(QJsonDocument(extra).toJson(QJsonDocument::Compact));
        feedback->setText(file.commit()
                        ? QObject::tr("Commande envoyée à l’agent…")
                        : QObject::tr("Échec de la commande."));
        Q_UNUSED(panel);
    };
    QObject::connect(apply, &QPushButton::clicked, panel, [=] {
        QJsonArray selected;
        for (int i = 0; i < playlists->count(); ++i)
            if (playlists->item(i)->checkState() == Qt::Checked)
                selected.append(
                        playlists->item(i)->data(Qt::UserRole).toString());
        QJsonArray blocks;
        for (int r = 0; r < schedule->rowCount(); ++r) {
            auto cell = [=](int c) {
                return schedule->item(r, c)
                        ? schedule->item(r, c)->text().trimmed()
                        : QString();
            };
            QJsonArray ids;
            for (const auto& id : cell(4).split(',', Qt::SkipEmptyParts))
                ids.append(id.trimmed());
            blocks.append(QJsonObject{{"id", cell(6)},
                    {"start", cell(0)},
                    {"end", cell(1)},
                    {"theme", cell(2)},
                    {"energyMode", cell(3)},
                    {"playlistIds", ids},
                    {"requiredTrackId", cell(5)}});
        }
        QJsonArray artists;
        for (const auto& artist :
                excluded->text().split(',', Qt::SkipEmptyParts))
            artists.append(artist.trimmed());
        send("configure",
                {{"config",
                        QJsonObject{{"mode", mode->currentData().toString()},
                                {"theme", theme->text()},
                                {"energyMode", energy->currentText()},
                                {"creativity", intensity->value() / 100.0},
                                {"requestWeight", weight->value() / 100.0},
                                {"stemsEnabled", stems->isChecked()},
                                {"mashupEnabled", mashup->isChecked()},
                                {"playlistIds", selected},
                                {"excludedArtists", artists},
                                {"blocks", blocks}}}});
    });
    QObject::connect(
            start, &QPushButton::clicked, panel, [=] { send("start"); });
    QObject::connect(next, &QPushButton::clicked, panel, [=] { send("next"); });
    QObject::connect(
            takeover, &QPushButton::clicked, panel, [=] { send("takeover"); });
    QObject::connect(lock, &QPushButton::clicked, panel, [=] {
        if (preview->currentRow() < 0)
            return;
        const auto item = preview->item(preview->currentRow(), 3);
        send("lock",
                {{"trackId", item->text()},
                        {"locked", !item->data(Qt::UserRole).toBool()}});
    });
    for (const auto& entry : {qMakePair(prepare, QStringLiteral("prepare-stems")), qMakePair(listen, QStringLiteral("preview"))})
        QObject::connect(entry.first, &QPushButton::clicked, panel, [=] {
            if (preview->currentRow() >= 0)
                send(entry.second, {{"trackId", preview->item(preview->currentRow(), 3)->text()}});
        });
    QObject::connect(cancel, &QPushButton::clicked, panel, [=] { send("cancel-stems"); });
    QObject::connect(stopListen, &QPushButton::clicked, panel, [=] { send("stop-preview"); });
    auto refresh = [=] {
        PollingControlProxy liveMode(QStringLiteral("[Gameflorr]"), QStringLiteral("live_mashup_mode"), ControlFlag::AllowMissingOrInvalid);
        PollingControlProxy liveState(QStringLiteral("[Gameflorr]"), QStringLiteral("live_mashup_status"), ControlFlag::AllowMissingOrInvalid);
        const int currentMode = static_cast<int>(liveMode.get());
        const int currentStatus = static_cast<int>(liveState.get());
        liveStatus->setText(liveMode.valid() ? liveMashupReason(currentStatus) : QObject::tr("Initialisation du mashup live…"));
        for (int i = 0; i < liveButtons->size(); ++i) {
            auto* button = liveButtons->at(i);
            button->setChecked(i < 2 && currentMode == i + 1);
            button->setEnabled(liveMode.valid() && (i == 3 || (currentStatus == 0 && (i < 2 || currentMode > 0))));
        }
        QFile file(QFileInfo(gameflorrStatePath())
                        .absoluteDir()
                        .filePath("mixxx-autodj-plan.json"));
        if (!file.open(QIODevice::ReadOnly) || file.size() > 2 * 1024 * 1024) {
            status->setText(QObject::tr(
                    "Agent Gameflorr hors ligne. Source Mixxx requise."));
            return;
        }
        *state = QJsonDocument::fromJson(file.readAll()).object();
        const bool online = state->value("version").toInt() == 1 &&
                QDateTime::currentMSecsSinceEpoch() -
                                state->value("updatedAt")
                                        .toVariant()
                                        .toLongLong() <
                        5000;
        const auto native = state->value("native").toObject();
        status->setText(!online
                        ? QObject::tr("Agent hors ligne — la lecture reste "
                                      "sous contrôle de Mixxx.")
                        : QObject::tr("%1 · %2 · %3")
                                  .arg(state->value("enabled").toBool()
                                                  ? QObject::tr(
                                                            "Conducteur actif")
                                                  : QObject::tr(
                                                            "Pilotage manuel"),
                                          state->value("theme").toString(),
                                          native.value("transition")
                                                  .toString()));
        const auto ack = state->value("lastCommand").toObject();
        if (!ack.isEmpty())
            feedback->setText(ack.value("ok").toBool()
                            ? QObject::tr("Commande appliquée.")
                            : ack.value("error").toString());
        if (!native.value("error").toString().isEmpty())
            feedback->setText(native.value("error").toString());
        if (*initialized != state->value("sessionId").toString()) {
            *initialized = state->value("sessionId").toString();
            const auto config = state->value("config").toObject();
            mode->setCurrentIndex(std::max(
                    0, mode->findData(config.value("mode").toString())));
            stems->setChecked(config.value("stemsEnabled").toBool());
            mashup->setChecked(config.value("mashupEnabled").toBool());
            theme->setText(config.value("theme").toString());
            energy->setCurrentText(config.value("energyMode").toString());
            intensity->setValue(
                    qRound(config.value("creativity").toDouble(.4) * 100));
            weight->setValue(
                    qRound(config.value("requestWeight").toDouble(.4) * 100));
            QStringList artists;
            for (const auto& v : config.value("excludedArtists").toArray())
                artists.append(v.toString());
            excluded->setText(artists.join(", "));
            schedule->setRowCount(0);
            for (const auto& v : config.value("blocks").toArray()) {
                const auto b = v.toObject();
                const int r = schedule->rowCount();
                schedule->insertRow(r);
                QStringList ids;
                for (const auto& id : b.value("playlistIds").toArray())
                    ids.append(id.toString());
                const QStringList cells = {b.value("start").toString(),
                        b.value("end").toString(),
                        b.value("theme").toString(),
                        b.value("energyMode").toString(),
                        ids.join(","),
                        b.value("requiredTrackId").toString(),
                        b.value("id").toString()};
                for (int c = 0; c < cells.size(); ++c)
                    schedule->setItem(r, c, new QTableWidgetItem(cells[c]));
            }
            playlists->clear();
        }
        QFile lists(QFileInfo(gameflorrStatePath())
                        .absoluteDir()
                        .filePath("musicdl-playlists.json"));
        if (lists.open(QIODevice::ReadOnly) && lists.size() < 2 * 1024 * 1024) {
            const auto selected = state->value("config")
                                          .toObject()
                                          .value("playlistIds")
                                          .toArray();
            for (const auto& v : QJsonDocument::fromJson(lists.readAll())
                            .object()
                            .value("playlists")
                            .toArray()) {
                const auto p = v.toObject();
                const auto id = p.value("id").toString();
                bool exists = false;
                for (int i = 0; i < playlists->count(); ++i)
                    if (playlists->item(i)->data(Qt::UserRole).toString() == id)
                        exists = true;
                if (exists)
                    continue;
                auto* item = new QListWidgetItem(
                        p.value("name").toString() + " · " + id, playlists);
                item->setData(Qt::UserRole, id);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(
                        selected.contains(id) ? Qt::Checked : Qt::Unchecked);
            }
        }
        const QString selection = preview->currentRow() >= 0
                ? preview->item(preview->currentRow(), 3)->text()
                : QString();
        preview->setRowCount(0);
        for (const auto& v : state->value("preview").toArray()) {
            const auto t = v.toObject();
            const int r = preview->rowCount();
            preview->insertRow(r);
            const QStringList cells = {t.value("title").toString() + " — " +
                            t.value("artist").toString(),
                    !t.value("stems").toObject().value("error").toString().isEmpty()
                            ? t.value("stems").toObject().value("error").toString()
                            : !t.value("stems").toObject().value("stage").toString().isEmpty()
                            ? t.value("stems").toObject().value("stage").toString()
                            : t.value("ready").toBool() ? QObject::tr("Audio prêt")
                                                        : QObject::tr("À télécharger"),
                    t.value("reason").toString(),
                    t.value("id").toString()};
            for (int c = 0; c < cells.size(); ++c)
                preview->setItem(r, c, new QTableWidgetItem(cells[c]));
            preview->item(r, 3)->setData(
                    Qt::UserRole, t.value("locked").toBool());
            if (t.value("locked").toBool())
                preview->item(r, 0)->setText(
                        QString::fromUtf8("🔒 ") + cells[0]);
            if (selection == cells[3])
                preview->selectRow(r);
        }
    };
    auto* timer = new QTimer(panel);
    QObject::connect(timer, &QTimer::timeout, panel, refresh);
    timer->start(1000);
    refresh();
    return scroll;
}
} // namespace gameflorr
