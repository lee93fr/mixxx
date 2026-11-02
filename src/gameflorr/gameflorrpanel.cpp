#include "gameflorr/gameflorrpanel.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QWizard>

#include "control/controlobject.h"
#include "controllers/controller.h"
#include "controllers/controllermanager.h"
#include "library/library.h"
#include "preferences/gameflorrsettings.h"
#include "soundio/soundmanager.h"
#include "sources/soundsourceproxy.h"

namespace gameflorr {
namespace {
class SetupPage : public QWizardPage {
  public:
    using QWizardPage::registerField;
};
void applyDarkPalette(QWidget* widget) {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#181a1e"));
    palette.setColor(QPalette::WindowText, QColor("#ededed"));
    palette.setColor(QPalette::Base, QColor("#101114"));
    palette.setColor(QPalette::AlternateBase, QColor("#202329"));
    palette.setColor(QPalette::Text, QColor("#ededed"));
    palette.setColor(QPalette::Button, QColor("#30343b"));
    palette.setColor(QPalette::ButtonText, QColor("#ededed"));
    palette.setColor(QPalette::Highlight, QColor("#13658b"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#747a83"));
    palette.setColor(
            QPalette::Disabled, QPalette::ButtonText, QColor("#747a83"));
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QStringLiteral(
            "QWidget { background: #181a1e; color: #ededed; }"
            "QLineEdit, QTableWidget { background: #101114; "
            "alternate-background-color: #202329; gridline-color: #30343b; "
            "selection-background-color: #13658b; }"
            "QHeaderView::section { background: #30343b; color: #ededed; "
            "padding: 4px; border: 0; }"
            "QPushButton { background: #30343b; border: 1px solid #555b65; "
            "border-radius: 3px; padding: 5px 9px; }"
            "QPushButton:hover { background: #414854; }"
            "QPushButton:disabled { color: #747a83; border-color: #30343b; }"));
}

QString text(const char* value) {
    return QCoreApplication::translate("Gameflorr", value);
}

QLabel* label(const QString& value, QVBoxLayout* layout) {
    auto* result = new QLabel(value);
    result->setTextFormat(Qt::PlainText);
    result->setWordWrap(true);
    layout->addWidget(result);
    return result;
}

QString stageText(const QString& stage) {
    if (stage == QStringLiteral("queued"))
        return text("En attente");
    if (stage == QStringLiteral("searching"))
        return text("Recherche…");
    if (stage == QStringLiteral("downloading"))
        return text("Téléchargement…");
    if (stage == QStringLiteral("ready") || stage == QStringLiteral("skipped"))
        return text("Prêt");
    return text("À vérifier dans l’agent");
}
} // namespace

QWizard* createSetupWizard(QWidget* parent,
        UserSettingsPointer settings,
        SoundManager* soundManager,
        ControllerManager* controllerManager,
        std::function<void(int)> openPreferences) {
    auto* wizard = new QWizard(parent);
    applyDarkPalette(wizard);
    wizard->setObjectName(QStringLiteral("GameflorrSetup"));
    wizard->setWindowTitle(text("Configurer Mixxx avec Gameflorr"));
    wizard->setWindowModality(Qt::NonModal);
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->setMinimumSize(620, 450);
    wizard->setAttribute(Qt::WA_DeleteOnClose);
    wizard->setButtonText(QWizard::NextButton, text("Suivant"));
    wizard->setButtonText(QWizard::BackButton, text("Précédent"));
    wizard->setButtonText(QWizard::CancelButton, text("Plus tard"));
    wizard->setButtonText(QWizard::FinishButton, text("Terminer"));
    auto page = [wizard](const QString& title) {
        auto* result = new SetupPage;
        result->setTitle(title);
        wizard->addPage(result);
        return result;
    };
    auto* welcome = page(text("Prêt à préparer ton mix ?"));
    auto* layout = new QVBoxLayout(welcome);
    label(text("Vérifie tes sorties audio, ton contrôleur et le partage "
               "Gameflorr. "
               "Tu peux revenir à cet assistant depuis le menu Gameflorr."),
            layout);
    label(text("Les réglages matériels s’ouvrent dans les préférences de "
               "Mixxx. "
               "Applique-les puis ferme les préférences pour revenir ici."),
            layout);
    layout->addStretch();

    auto* audio = page(text("1 · Enceintes et préécoute"));
    layout = new QVBoxLayout(audio);
    auto* audioStatus = label({}, layout);
    label(text("Choisis la sortie principale et une sortie casque distincte. "
               "Avec une table externe, vérifie plutôt les sorties de chaque "
               "platine. "
               "Commence à faible volume."),
            layout);
    auto* audioButton = new QPushButton(text("Régler les sorties audio…"));
    audioButton->setObjectName(QStringLiteral("setupAudio"));
    layout->addWidget(audioButton);
    QObject::connect(
            audioButton, &QPushButton::clicked, wizard, [openPreferences] {
                openPreferences(0);
            });
    label(text("Charge ensuite un morceau dans Mixxx, lance-le et vérifie les "
               "enceintes. "
               "Active PFL sur l’autre platine pour vérifier le casque. "
               "L’assistant ne peut pas confirmer le son réellement entendu."),
            layout);
    layout->addStretch();

    auto* controllers = page(text("2 · Contrôleur"));
    layout = new QVBoxLayout(controllers);
    auto* controllerStatus = label({}, layout);
    label(text("Branche ton contrôleur, choisis son mapping et active-le. "
               "Tu peux aussi mixer avec la souris et le clavier."),
            layout);
    auto* controllerButton = new QPushButton(text("Configurer le contrôleur…"));
    controllerButton->setObjectName(QStringLiteral("setupController"));
    layout->addWidget(controllerButton);
    QObject::connect(
            controllerButton, &QPushButton::clicked, wizard, [openPreferences] {
                openPreferences(1);
            });
    label(text("RANE ONE : sélectionne le preset expérimental. Les plateaux "
               "motorisés "
               "et le scratch ne sont pas encore pris en charge."),
            layout);
    layout->addStretch();

    auto* sharing = page(text("3 · Connecter Gameflorr"));
    layout = new QVBoxLayout(sharing);
    auto* enabled =
            new QCheckBox(text("Partager le titre joué avec Gameflorr"));
    const bool forced =
            !qEnvironmentVariableIsEmpty("GAMEFLORR_MIXXX_STATE_PATH");
    enabled->setChecked(forced ||
            settings->getValue(ConfigKey("[Gameflorr]", "enabled"), false));
    enabled->setEnabled(!forced);
    layout->addWidget(enabled);
    auto* apply = new QPushButton(text("Appliquer le partage"));
    apply->setEnabled(!forced);
    layout->addWidget(apply);
    QObject::connect(apply, &QPushButton::clicked, wizard, [settings, enabled] {
        settings->setValue(
                ConfigKey("[Gameflorr]", "enabled"), enabled->isChecked());
        settings->save();
    });
    label(text("Dans l’agent Gameflorr, renseigne la clé de ta soirée, choisis "
               "la source "
               "Mixxx et le mode DJ, puis démarre l’agent. Configure son API "
               "musique "
               "pour télécharger les demandes."),
            layout);
    auto* path = new QLineEdit(QDir::toNativeSeparators(gameflorrStatePath()));
    path->setReadOnly(true);
    layout->addWidget(path);
    label(text("Les deux applications doivent utiliser ce même fichier local. "
               "Pendant une transition à deux platines, le dernier titre "
               "publié reste affiché."),
            layout);
    auto* bridgeStatus = label({}, layout);
    layout->addStretch();

    auto* checks = page(text("4 · Vérifier avant de mixer"));
    layout = new QVBoxLayout(checks);
    label(text("Laisse l’assistant ouvert et utilise Mixxx pour ces essais. "
               "Coche uniquement les vérifications que tu as effectuées."),
            layout);
    auto* mainCheck =
            new QCheckBox(text("J’ai vérifié le son sur les enceintes"));
    auto* cueCheck = new QCheckBox(text("J’ai vérifié la préécoute au casque"));
    auto* controlCheck = new QCheckBox(
            text("J’ai vérifié la lecture, les faders et le chargement"));
    layout->addWidget(mainCheck);
    layout->addWidget(cueCheck);
    layout->addWidget(controlCheck);
    checks->registerField(QStringLiteral("mainChecked*"), mainCheck);
    checks->registerField(QStringLiteral("cueChecked*"), cueCheck);
    checks->registerField(QStringLiteral("controlsChecked*"), controlCheck);
    label(text("Les demandes du public se trouvent dans Gameflorr → Demandes "
               "du public. "
               "Le chargement reste manuel et est bloqué sur une platine en "
               "lecture."),
            layout);
    layout->addStretch();
    QObject::connect(wizard, &QWizard::accepted, wizard, [settings] {
        settings->setValue(ConfigKey("[Gameflorr]", "setupCompleted"), true);
        settings->save();
    });
    auto refresh = [audioStatus,
                           controllerStatus,
                           bridgeStatus,
                           soundManager,
                           controllerManager] {
        QStringList outputs;
        const auto config = soundManager->getConfig();
        const auto configured = config.getOutputs();
        for (auto it = configured.cbegin(); it != configured.cend(); ++it) {
            outputs.append(it.value().getString());
        }
        audioStatus->setText(outputs.isEmpty()
                        ? text("Aucune sortie configurée.")
                        : text("Sorties configurées : ") +
                                outputs.join(QStringLiteral(", ")));
        QStringList devices;
        for (auto* controller :
                controllerManager->getControllerList(false, true)) {
            devices.append(controller->getName());
        }
        controllerStatus->setText(devices.isEmpty()
                        ? text("Aucun contrôleur détecté.")
                        : text("Périphériques détectés : ") +
                                devices.join(QStringLiteral(", ")));
        QFile file(requestsPath());
        std::optional<RequestSnapshot> snapshot;
        if (file.open(QIODevice::ReadOnly) && file.size() <= 2 * 1024 * 1024) {
            snapshot = parseRequestSnapshot(
                    file.readAll(), QDateTime::currentMSecsSinceEpoch());
        }
        bridgeStatus->setText(snapshot && snapshot->online
                        ? (snapshot->backendConnected
                                          ? text("Agent connecté à Gameflorr.")
                                          : text("Agent présent · soirée "
                                                 "déconnectée."))
                        : text("En attente de l’agent Mixxx sur cet "
                               "ordinateur."));
    };
    auto* timer = new QTimer(wizard);
    QObject::connect(timer, &QTimer::timeout, wizard, refresh);
    timer->start(1000);
    refresh();
    return wizard;
}

RequestsPanel::RequestsPanel(QWidget* parent, Library* library)
        : QWidget(parent),
          m_pLibrary(library) {
    applyDarkPalette(this);
    setObjectName(QStringLiteral("GameflorrRequests"));
    auto* layout = new QVBoxLayout(this);
    m_pStatus = label(text("En attente de l’agent Gameflorr…"), layout);
    m_pFilter = new QLineEdit;
    m_pFilter->setPlaceholderText(
            text("Filtrer les demandes par titre ou artiste"));
    layout->addWidget(m_pFilter);
    m_pTable = new QTableWidget(0, 3);
    m_pTable->setHorizontalHeaderLabels(
            {text("Titre"), text("Artiste"), text("État")});
    m_pTable->setAlternatingRowColors(true);
    m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pTable->verticalHeader()->hide();
    layout->addWidget(m_pTable);
    auto* actions = new QHBoxLayout;
    auto* search = new QPushButton(text("Chercher dans la bibliothèque"));
    m_pDownload = new QPushButton(text("Télécharger / réessayer"));
    m_pLoad1 = new QPushButton(text("Charger sur 1"));
    m_pLoad2 = new QPushButton(text("Charger sur 2"));
    actions->addWidget(search);
    actions->addWidget(m_pDownload);
    actions->addWidget(m_pLoad1);
    actions->addWidget(m_pLoad2);
    layout->addLayout(actions);
    m_pFeedback = label({}, layout);
    connect(search, &QPushButton::clicked, this, [this] {
        if (const auto* request = selectedRequest()) {
            m_pLibrary->searchTracksInCollection(request->title);
        }
    });
    connect(m_pDownload, &QPushButton::clicked, this, [this] { download(); });
    connect(m_pLoad1, &QPushButton::clicked, this, [this] { load(1); });
    connect(m_pLoad2, &QPushButton::clicked, this, [this] { load(2); });
    connect(m_pTable, &QTableWidget::itemSelectionChanged, this, [this] {
        updateActions();
    });
    connect(m_pFilter, &QLineEdit::textChanged, this, [this] {
        for (int row = 0; row < m_pTable->rowCount(); ++row) {
            const QString value = m_pTable->item(row, 0)->text() +
                    m_pTable->item(row, 1)->text();
            m_pTable->setRowHidden(row,
                    !value.contains(m_pFilter->text(), Qt::CaseInsensitive));
        }
        updateActions();
    });
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        if (isVisible())
            refresh();
    });
    timer->start(500);
    refresh();
}

const Request* RequestsPanel::selectedRequest() const {
    if (!m_snapshot || m_pTable->currentRow() < 0 ||
            m_pTable->isRowHidden(m_pTable->currentRow()))
        return nullptr;
    const auto* item = m_pTable->item(m_pTable->currentRow(), 0);
    if (!item)
        return nullptr;
    const QString id = item->data(Qt::UserRole).toString();
    for (const auto& request : m_snapshot->requests) {
        if (request.id == id)
            return &request;
    }
    return nullptr;
}

void RequestsPanel::refresh() {
    QFile file(requestsPath());
    std::optional<RequestSnapshot> snapshot;
    if (file.open(QIODevice::ReadOnly) && file.size() <= 2 * 1024 * 1024) {
        snapshot = parseRequestSnapshot(
                file.readAll(), QDateTime::currentMSecsSinceEpoch());
    }
    QString selection;
    if (const auto* request = selectedRequest())
        selection = request->id;
    m_snapshot = std::move(snapshot);
    if (!m_snapshot || !m_snapshot->online) {
        m_pStatus->setText(
                text("Agent arrêté ou indisponible · démarre l’agent avec la "
                     "source Mixxx et le mode DJ."));
        updateActions();
        return;
    }
    m_pStatus->setText(m_snapshot->backendConnected
                    ? (m_snapshot->requests.isEmpty()
                                      ? text("Connecté · aucune demande en "
                                             "attente.")
                                      : text("Connecté à Gameflorr · "
                                             "sélectionne une demande."))
                    : text("Agent présent · connexion à la soirée "
                           "interrompue."));
    if (!m_snapshot->downloadsEnabled) {
        m_pStatus->setText(m_pStatus->text() +
                text(" Configure l’API musique dans l’agent pour "
                     "télécharger."));
    }
    if (!m_pendingCommand.isEmpty()) {
        if (m_snapshot->lastCommand.value(QStringLiteral("id")).toString() ==
                m_pendingCommand) {
            m_pFeedback->setText(
                    m_snapshot->lastCommand.value(QStringLiteral("ok")).toBool()
                            ? text("Téléchargement demandé à l’agent.")
                            : m_snapshot->lastCommand
                                      .value(QStringLiteral("error"))
                                      .toString());
            m_pendingCommand.clear();
        } else if (QDateTime::currentMSecsSinceEpoch() - m_commandSentAt >
                10000) {
            m_pFeedback->setText(
                    text("L’agent n’a pas confirmé la demande. Vérifie sa "
                         "connexion puis réessaie."));
            m_pendingCommand.clear();
        }
    }
    QJsonArray contents;
    for (const auto& request : m_snapshot->requests) {
        contents.append(QJsonArray{request.id,
                request.title,
                request.artist,
                request.stage,
                request.filePath,
                request.error});
    }
    const QByteArray data = m_snapshot->sessionId.toUtf8() +
            QJsonDocument(contents).toJson(QJsonDocument::Compact);
    if (data != m_lastData) {
        m_lastData = data;
        const QSignalBlocker blocker(m_pTable);
        m_pTable->clearSelection();
        m_pTable->setCurrentCell(-1, -1);
        m_pTable->setRowCount(m_snapshot->requests.size());
        for (int row = 0; row < m_snapshot->requests.size(); ++row) {
            const auto& request = m_snapshot->requests[row];
            auto* title = new QTableWidgetItem(request.title);
            title->setData(Qt::UserRole, request.id);
            m_pTable->setItem(row, 0, title);
            m_pTable->setItem(row, 1, new QTableWidgetItem(request.artist));
            auto* state = new QTableWidgetItem(stageText(request.stage));
            state->setToolTip(request.error);
            m_pTable->setItem(row, 2, state);
            m_pTable->setRowHidden(row,
                    !QString(request.title + request.artist)
                            .contains(m_pFilter->text(), Qt::CaseInsensitive));
            if (request.id == selection)
                m_pTable->selectRow(row);
        }
    }
    updateActions();
}

void RequestsPanel::updateActions() {
    const auto* request = selectedRequest();
    const bool online = m_snapshot && m_snapshot->online;
    m_pDownload->setEnabled(online && request && m_snapshot->backendConnected &&
            m_snapshot->downloadsEnabled && m_pendingCommand.isEmpty() &&
            (request->stage == QStringLiteral("queued") ||
                    request->stage == QStringLiteral("error")));
    for (int deck = 1; deck <= 2; ++deck) {
        const QString group = QStringLiteral("[Channel%1]").arg(deck);
        const bool allowed = online && request &&
                canLoadRequest(*request,
                        ControlObject::get(ConfigKey(group, "play")) > 0,
                        ControlObject::get(ConfigKey(group, "passthrough")) >
                                0);
        (deck == 1 ? m_pLoad1 : m_pLoad2)->setEnabled(allowed);
    }
}

void RequestsPanel::load(int deck) {
    refresh();
    const auto* request = selectedRequest();
    const QString group = QStringLiteral("[Channel%1]").arg(deck);
    if (!m_snapshot || !m_snapshot->online || !request ||
            !canLoadRequest(*request,
                    ControlObject::get(ConfigKey(group, "play")) > 0,
                    ControlObject::get(ConfigKey(group, "passthrough")) > 0)) {
        m_pFeedback->setText(
                text("Chargement bloqué : vérifie le fichier et arrête la "
                     "platine de destination."));
        return;
    }
    if (!SoundSourceProxy::isFileNameSupported(request->filePath)) {
        m_pFeedback->setText(text(
                "Format audio non pris en charge par cette installation."));
        return;
    }
    m_pLibrary->slotLoadLocationToPlayer(request->filePath, group, false);
    m_pFeedback->setText(text(
            "Chargement demandé sur la platine %1, sans démarrer la lecture.")
                    .arg(deck));
}

void RequestsPanel::download() {
    refresh();
    const auto* request = selectedRequest();
    if (!request || !m_pDownload->isEnabled())
        return;
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QJsonObject command{{QStringLiteral("version"), 1},
            {QStringLiteral("sessionId"), m_snapshot->sessionId},
            {QStringLiteral("id"), id},
            {QStringLiteral("updatedAt"), static_cast<double>(now)},
            {QStringLiteral("action"), QStringLiteral("download")},
            {QStringLiteral("requestId"), request->id}};
    QSaveFile file(commandPath());
    const auto data = QJsonDocument(command).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) ||
            !file.setPermissions(
                    QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
            file.write(data) != data.size() || !file.commit()) {
        m_pFeedback->setText(
                text("Impossible d’envoyer la demande à l’agent. Vérifie le "
                     "dossier du pont local."));
        return;
    }
    m_pendingCommand = id;
    m_commandSentAt = now;
    m_pFeedback->setText(text("En attente de confirmation de l’agent…"));
    updateActions();
}
} // namespace gameflorr
