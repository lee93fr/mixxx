#include "gameflorr/gameflorrpanel.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

namespace {
QString settingsBridgePath(const QString& name) {
    return QFileInfo(gameflorrStatePath()).absoluteDir().filePath(name);
}

class BridgeDialog : public QDialog {
  public:
    explicit BridgeDialog(QWidget* parent)
            : QDialog(parent) {
        setAttribute(Qt::WA_DeleteOnClose);
        applyDarkPalette(this);
        m_layout = new QVBoxLayout(this);
        m_status = label({}, m_layout);
        auto* timer = new QTimer(this);
        QObject::connect(timer, &QTimer::timeout, this, [this] { refresh(); });
        timer->start(500);
    }

    ~BridgeDialog() override {
        removePendingCommand();
    }

  protected:
    void refresh() {
        QFile file(settingsBridgePath(QStringLiteral("gameflorr-settings.json")));
        m_snapshot = {};
        if (file.open(QIODevice::ReadOnly) && file.size() <= 64 * 1024) {
            const auto candidate = QJsonDocument::fromJson(file.readAll()).object();
            const auto updated = candidate.value(QStringLiteral("updatedAt")).toDouble();
            const auto now = QDateTime::currentMSecsSinceEpoch();
            if (candidate.value(QStringLiteral("version")).toInt() == 1 &&
                    candidate.value(QStringLiteral("online")).toBool() &&
                    !candidate.value(QStringLiteral("sessionId")).toString().isEmpty() &&
                    updated > 0 && now - updated <= 5000 && updated <= now + 2000) {
                m_snapshot = candidate;
            }
        }
        if (!m_pending.isEmpty()) {
            const auto result = m_snapshot.value(QStringLiteral("lastCommand")).toObject();
            if (result.value(QStringLiteral("id")).toString() == m_pending) {
                m_status->setText(result.value(QStringLiteral("message")).toString());
                m_pending.clear();
                commandFinished(result.value(QStringLiteral("ok")).toBool());
            } else if (QDateTime::currentMSecsSinceEpoch() - m_sentAt > 15000) {
                removePendingCommand();
                m_pending.clear();
                m_status->setText(text("Aucune réponse. Vérifiez que l’agent est ouvert."));
            }
        }
        if (m_snapshot.isEmpty()) {
            m_status->setText(
                    text("Ouvrez Gameflorr Agent sur cet ordinateur pour régler les connexions."));
        } else if (!m_pending.isEmpty()) {
            m_status->setText(text("Commande en cours…"));
        } else if (m_status->text().isEmpty() || m_wasOffline) {
            m_status->setText(
                    text("Agent disponible. Les clés déjà enregistrées sont conservées si les "
                         "champs restent vides."));
        }
        m_wasOffline = m_snapshot.isEmpty();
        updateState();
    }

    bool available() const {
        return !m_snapshot.isEmpty() && m_pending.isEmpty() &&
                !m_snapshot.value(QStringLiteral("busy")).toBool();
    }

    void send(const QString& action, QJsonObject payload = {}) {
        if (!available()) {
            return;
        }
        const auto path = settingsBridgePath(QStringLiteral("gameflorr-settings-command.json"));
        if (QFileInfo::exists(path)) {
            m_status->setText(text("Une autre commande est en cours. Réessayez dans un instant."));
            return;
        }
        m_pending = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_sentAt = QDateTime::currentMSecsSinceEpoch();
        payload.insert(QStringLiteral("version"), 1);
        payload.insert(QStringLiteral("id"), m_pending);
        payload.insert(QStringLiteral("sessionId"), m_snapshot.value(QStringLiteral("sessionId")));
        payload.insert(QStringLiteral("createdAt"), m_sentAt);
        payload.insert(QStringLiteral("action"), action);
        QSaveFile file(path);
        const auto bytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        if (!file.open(QIODevice::WriteOnly)) {
            m_pending.clear();
            m_status->setText(text("Impossible d’écrire les réglages pour l’agent."));
            return;
        }
        if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
                file.write(bytes) != bytes.size() || !file.commit()) {
            m_pending.clear();
            m_status->setText(text("Impossible de transmettre les réglages privés."));
            return;
        }
        refresh();
    }

    virtual void updateState() = 0;
    virtual void commandFinished(bool) {
    }
    QVBoxLayout* m_layout;
    QLabel* m_status;
    QJsonObject m_snapshot;

  private:
    void removePendingCommand() {
        if (m_pending.isEmpty()) {
            return;
        }
        const auto path = settingsBridgePath(QStringLiteral("gameflorr-settings-command.json"));
        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.size() <= 32 * 1024 &&
                QJsonDocument::fromJson(file.readAll())
                                .object()
                                .value(QStringLiteral("id"))
                                .toString() == m_pending) {
            file.close();
            QFile::remove(path);
        }
    }
    QString m_pending;
    qint64 m_sentAt = 0;
    bool m_wasOffline = true;
};

class SettingsDialog : public BridgeDialog {
  public:
    SettingsDialog(QWidget* parent,
            UserSettingsPointer settings,
            std::function<void(bool)> setScreenButtonVisible)
            : BridgeDialog(parent) {
        setWindowTitle(text("Gameflorr · Paramètres et clés API"));
        setObjectName(QStringLiteral("GameflorrSettings"));
        setMinimumWidth(610);
        auto* form = new QFormLayout;
        m_layout->addLayout(form);
        auto addField = [this, form](const QString& title, const char* name, bool secret = false) {
            auto* field = new QLineEdit(this);
            field->setObjectName(QString::fromLatin1(name));
            field->setMaxLength(4096);
            if (secret) {
                field->setEchoMode(QLineEdit::Password);
                field->setPlaceholderText(text("Laisser vide pour conserver la clé"));
            }
            form->addRow(title, field);
            return field;
        };
        m_api = addField(text("API Gameflorr"), "gameflorrApiUrl");
        m_event = addField(text("ID événement (facultatif)"), "gameflorrEventId");
        m_key = addField(text("Clé de l’événement"), "gameflorrAgentKey", true);
        m_musicApi = addField(text("API MusicDL"), "musicdlApiUrl");
        m_musicKey = addField(text("Jeton MusicDL"), "musicdlApiToken", true);
        m_credentials = label({}, m_layout);
        auto* visibility =
                new QCheckBox(text("Afficher le bouton « Écran Gameflorr » dans le panneau"));
        visibility->setObjectName(QStringLiteral("showGameflorrScreenButton"));
        visibility->setChecked(
                settings->getValue(ConfigKey("[Gameflorr]", "showScreenButton"), true));
        m_layout->addWidget(visibility);
        QObject::connect(visibility,
                &QCheckBox::toggled,
                this,
                [settings, setScreenButtonVisible](bool enabled) {
                    settings->setValue(ConfigKey("[Gameflorr]", "showScreenButton"), enabled);
                    settings->save();
                    setScreenButtonVisible(enabled);
                });
        label(text("Les clés restent dans la configuration privée de l’agent. "
                   "Enregistrer démarre ou reconnecte l’agent en mode Mixxx. "
                   "Les tests utilisent les champs saisis sans les enregistrer."),
                m_layout);
        auto* actions = new QHBoxLayout;
        m_layout->addLayout(actions);
        m_testEvent = new QPushButton(text("Tester l’événement"));
        m_testMusic = new QPushButton(text("Tester MusicDL"));
        m_save = new QPushButton(text("Enregistrer et appliquer"));
        m_save->setObjectName(QStringLiteral("saveGameflorrSettings"));
        auto* close = new QPushButton(text("Fermer"));
        for (auto* button : {m_testEvent, m_testMusic, m_save, close}) {
            actions->addWidget(button);
        }
        QObject::connect(close, &QPushButton::clicked, this, &QDialog::close);
        QObject::connect(m_testEvent, &QPushButton::clicked, this, [this] {
            send(QStringLiteral("test-event"), {{QStringLiteral("config"), config()}});
        });
        QObject::connect(m_testMusic, &QPushButton::clicked, this, [this] {
            send(QStringLiteral("test-music"), {{QStringLiteral("config"), config()}});
        });
        QObject::connect(m_save, &QPushButton::clicked, this, [this] {
            m_saving = true;
            send(QStringLiteral("save"), {{QStringLiteral("config"), config()}});
        });
        refresh();
    }

  private:
    QJsonObject config() const {
        QJsonObject result{{QStringLiteral("apiUrl"), m_api->text().trimmed()},
                {QStringLiteral("eventId"), m_event->text().trimmed()},
                {QStringLiteral("musicDownloadApiUrl"), m_musicApi->text().trimmed()}};
        if (!m_key->text().isEmpty()) {
            result.insert(QStringLiteral("agentKey"), m_key->text().trimmed());
        }
        if (!m_musicKey->text().isEmpty()) {
            result.insert(QStringLiteral("musicDownloadApiToken"), m_musicKey->text().trimmed());
        }
        return result;
    }
    void updateState() override {
        const auto cfg = m_snapshot.value(QStringLiteral("config")).toObject();
        if (!m_initialized && !cfg.isEmpty()) {
            m_api->setText(cfg.value(QStringLiteral("apiUrl")).toString());
            m_event->setText(cfg.value(QStringLiteral("eventId")).toString());
            m_musicApi->setText(cfg.value(QStringLiteral("musicDownloadApiUrl")).toString());
            m_musicApi->setPlaceholderText(QStringLiteral("https://musicdl.leelyan.ovh/api"));
            m_initialized = true;
        }
        m_credentials->setText(text("Clé événement : %1 · Jeton MusicDL : %2")
                        .arg(cfg.value(QStringLiteral("agentKeyConfigured")).toBool()
                                        ? text("enregistrée")
                                        : text("à renseigner"),
                                cfg.value(QStringLiteral("musicTokenConfigured")).toBool()
                                        ? text("enregistré")
                                        : text("à renseigner")));
        for (auto* button : {m_testEvent, m_testMusic, m_save}) {
            button->setEnabled(available());
        }
    }
    void commandFinished(bool ok) override {
        if (m_saving && ok) {
            m_key->clear();
            m_musicKey->clear();
        }
        m_saving = false;
    }
    QLineEdit *m_api, *m_event, *m_key, *m_musicApi, *m_musicKey;
    QLabel* m_credentials;
    QPushButton *m_testEvent, *m_testMusic, *m_save;
    bool m_initialized = false;
    bool m_saving = false;
};

class DisplayDialog : public BridgeDialog {
  public:
    explicit DisplayDialog(QWidget* parent)
            : BridgeDialog(parent) {
        setWindowTitle(text("Écran public Gameflorr"));
        setObjectName(QStringLiteral("GameflorrDisplay"));
        setMinimumWidth(540);
        m_current = label(text("Actualisez pour consulter l’écran de l’événement."), m_layout);
        m_modes = new QComboBox;
        m_modes->setObjectName(QStringLiteral("gameflorrDisplayMode"));
        const QList<QPair<const char*, const char*>> modes{{"NOW_PLAYING", "Titre en cours"},
                {"VOTE", "Votes musicaux"},
                {"REACTIONS", "Réactions"},
                {"DEDICATION", "Dédicaces"},
                {"PHOTOS", "Photos"},
                {"WALL", "Mur de photos"},
                {"PHOTO_PODIUM", "Podium photos"},
                {"LEADERBOARD", "Classement"},
                {"SUMMARY", "Résumé"},
                {"PROGRAM", "Programme"},
                {"ROTATE", "Rotation configurée dans Gameflorr"}};
        for (const auto& mode : modes) {
            m_modes->addItem(text(mode.second), QString::fromLatin1(mode.first));
        }
        m_layout->addWidget(m_modes);
        label(text("Appliquer change immédiatement le contenu de l’écran public. "
                   "La rotation reprend la sélection configurée dans Gameflorr."),
                m_layout);
        auto* actions = new QHBoxLayout;
        m_layout->addLayout(actions);
        m_reload = new QPushButton(text("Actualiser"));
        m_apply = new QPushButton(text("Appliquer à l’écran public"));
        m_apply->setObjectName(QStringLiteral("applyGameflorrDisplay"));
        auto* close = new QPushButton(text("Fermer"));
        for (auto* button : {m_reload, m_apply, close}) {
            actions->addWidget(button);
        }
        QObject::connect(close, &QPushButton::clicked, this, &QDialog::close);
        QObject::connect(m_reload, &QPushButton::clicked, this, [this] {
            send(QStringLiteral("display-refresh"));
        });
        QObject::connect(m_apply, &QPushButton::clicked, this, [this] {
            send(QStringLiteral("display-set"),
                    {{QStringLiteral("mode"), m_modes->currentData().toString()}});
        });
        refresh();
        send(QStringLiteral("display-refresh"));
    }

  private:
    void updateState() override {
        const auto display = m_snapshot.value(QStringLiteral("display")).toObject();
        if (!display.isEmpty()) {
            const int index = m_modes->findData(display.value(QStringLiteral("mode")).toString());
            const QString mode = index >= 0 ? m_modes->itemText(index)
                                            : display.value(QStringLiteral("mode")).toString();
            m_current->setText(text("%1 · Écran actuel : %2")
                            .arg(display.value(QStringLiteral("eventName")).toString(), mode));
            if (!m_initialized && index >= 0) {
                m_modes->setCurrentIndex(index);
                m_initialized = true;
            }
        }
        const bool ready = available() && !display.isEmpty();
        m_apply->setEnabled(ready);
        m_modes->setEnabled(ready);
        m_reload->setEnabled(available());
    }
    QLabel* m_current;
    QComboBox* m_modes;
    QPushButton *m_reload, *m_apply;
    bool m_initialized = false;
};
} // namespace

QDialog* createSettingsDialog(QWidget* parent,
        UserSettingsPointer settings,
        std::function<void(bool)> setScreenButtonVisible) {
    return new SettingsDialog(parent, settings, std::move(setScreenButtonVisible));
}

QDialog* createDisplayDialog(QWidget* parent) {
    return new DisplayDialog(parent);
}

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

MusicdlPanel::MusicdlPanel(QWidget* parent, Library* library)
        : QWidget(parent),
          m_pLibrary(library) {
    applyDarkPalette(this);
    setObjectName(QStringLiteral("MusicdlPlaylists"));
    auto* layout = new QVBoxLayout(this);
    m_pStatus = label({}, layout);
    m_pFilter = new QLineEdit;
    m_pFilter->setPlaceholderText(text("Filtrer les playlists MusicDL"));
    layout->addWidget(m_pFilter);
    m_pTable = new QTableWidget(0, 4);
    m_pTable->setMinimumHeight(120);
    m_pTable->setHorizontalHeaderLabels(
            {text("Playlist"), text("Dossier"), text("Morceaux"), text("Préparation")});
    m_pTable->setAlternatingRowColors(true);
    m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pTable->verticalHeader()->hide();
    layout->addWidget(m_pTable);
    auto* actions = new QHBoxLayout;
    m_pRefresh = new QPushButton(text("Actualiser MusicDL"));
    m_pPrepare = new QPushButton(text("Télécharger la playlist"));
    m_pImport = new QPushButton(text("Importer dans Mixxx"));
    actions->addWidget(m_pRefresh);
    actions->addWidget(m_pPrepare);
    actions->addWidget(m_pImport);
    layout->addLayout(actions);
    m_pFeedback = label({}, layout);
    connect(m_pRefresh, &QPushButton::clicked, this, [this] {
        sendCommand(QStringLiteral("refresh"));
    });
    connect(m_pPrepare, &QPushButton::clicked, this, [this] {
        sendCommand(QStringLiteral("prepare"));
    });
    connect(m_pImport, &QPushButton::clicked, this, [this] {
        const auto playlist = selectedPlaylist();
        const QString path = playlist.value(QStringLiteral("m3uPath")).toString();
        const QFileInfo file(path);
        const QDir expected(QFileInfo(requestsPath())
                        .absoluteDir()
                        .filePath(QStringLiteral("musicdl-playlists")));
        if (m_importedPaths.contains(path) || path.isEmpty() || !file.isAbsolute() ||
                !file.isFile() || file.suffix() != QStringLiteral("m3u8") ||
                file.canonicalPath() != expected.canonicalPath()) {
            m_pFeedback->setText(text("Playlist locale indisponible. Télécharge-la à nouveau."));
            return;
        }
        if (m_pLibrary->importPlaylistFile(path,
                    QStringLiteral("MusicDL · ") +
                            playlist.value(QStringLiteral("name")).toString())) {
            m_importedPaths.insert(path);
            m_pFeedback->setText(text("Playlist importée dans la bibliothèque Mixxx."));
        } else {
            m_pFeedback->setText(text("L’import de la playlist a échoué."));
        }
        updateActions();
    });
    connect(m_pTable, &QTableWidget::itemSelectionChanged, this, [this] { updateActions(); });
    connect(m_pFilter, &QLineEdit::textChanged, this, [this] { refresh(); });
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        if (isVisible()) {
            refresh();
        }
    });
    timer->start(500);
    refresh();
}

QJsonObject MusicdlPanel::selectedPlaylist() const {
    const auto* item = m_pTable->item(m_pTable->currentRow(), 0);
    if (!item || m_pTable->isRowHidden(m_pTable->currentRow())) {
        return {};
    }
    const QString id = item->data(Qt::UserRole).toString();
    for (const auto& value : m_snapshot.value(QStringLiteral("playlists")).toArray()) {
        const auto playlist = value.toObject();
        if (playlist.value(QStringLiteral("id")).toString() == id) {
            return playlist;
        }
    }
    return {};
}

void MusicdlPanel::refresh() {
    const QString selection = selectedPlaylist().value(QStringLiteral("id")).toString();
    QFile file(QFileInfo(requestsPath())
                    .absoluteDir()
                    .filePath(QStringLiteral("musicdl-playlists.json")));
    QJsonObject snapshot;
    if (file.open(QIODevice::ReadOnly) && file.size() <= 8 * 1024 * 1024) {
        snapshot = QJsonDocument::fromJson(file.readAll()).object();
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double updatedAt = snapshot.value(QStringLiteral("updatedAt")).toDouble();
    const bool online = snapshot.value(QStringLiteral("version")).toInt() == 1 &&
            !snapshot.value(QStringLiteral("sessionId")).toString().isEmpty() &&
            snapshot.value(QStringLiteral("online")).toBool() && updatedAt <= now + 2000 &&
            updatedAt >= now - 5000;
    if (!online) {
        snapshot = {};
    }
    m_snapshot = snapshot;
    QString status;
    if (!online) {
        status =
                text("Démarre l’agent Gameflorr avec la source Mixxx et le mode DJ pour accéder à "
                     "MusicDL.");
    } else if (!snapshot.value(QStringLiteral("configured")).toBool()) {
        status = text(
                "Configure l’URL MusicDL et ton jeton personnel dans les réglages de l’agent.");
    } else if (snapshot.value(QStringLiteral("busy")).toBool()) {
        status = text("Lecture des playlists MusicDL…");
    } else if (!snapshot.value(QStringLiteral("error")).toString().isEmpty()) {
        status = snapshot.value(QStringLiteral("error")).toString();
    } else {
        status =
                text("MusicDL connecté · choisis une playlist, télécharge ses morceaux puis "
                     "importe-la dans Mixxx.");
    }
    m_pStatus->setText(status);
    if (!m_pendingCommand.isEmpty()) {
        const auto result = snapshot.value(QStringLiteral("lastCommand")).toObject();
        if (result.value(QStringLiteral("id")).toString() == m_pendingCommand) {
            m_pFeedback->setText(result.value(QStringLiteral("ok")).toBool()
                            ? text("Demande acceptée. La progression s’affiche dans la liste.")
                            : result.value(QStringLiteral("error")).toString());
            m_pendingCommand.clear();
        } else if (!online || now - m_commandSentAt > 120000) {
            m_pFeedback->setText(
                    text("L’agent n’a pas confirmé la demande. Actualise avant de réessayer."));
            m_pendingCommand.clear();
        }
    }
    const auto playlists = snapshot.value(QStringLiteral("playlists")).toArray();
    const auto folders = snapshot.value(QStringLiteral("folders")).toArray();
    QSignalBlocker blocker(m_pTable);
    m_pTable->setRowCount(qMin<int>(playlists.size(), 5000));
    for (int row = 0; row < m_pTable->rowCount(); ++row) {
        const auto playlist = playlists[row].toObject();
        const QString id = playlist.value(QStringLiteral("id")).toString();
        QString folderName;
        for (const auto& folder : folders) {
            if (folder.toObject().value(QStringLiteral("id")) ==
                    playlist.value(QStringLiteral("folder_id"))) {
                folderName = folder.toObject().value(QStringLiteral("name")).toString();
                break;
            }
        }
        const QString name = playlist.value(QStringLiteral("name")).toString();
        const QString path = playlist.value(QStringLiteral("m3uPath")).toString();
        const int total = playlist.value(QStringLiteral("track_count")).toInt();
        const int ready = playlist.value(QStringLiteral("ready")).toInt();
        const int failed = playlist.value(QStringLiteral("failed")).toInt();
        QString progress = QStringLiteral("%1 / %2").arg(ready).arg(total);
        if (failed) {
            progress += text(" · %1 échec(s)").arg(failed);
        } else if (m_importedPaths.contains(path)) {
            progress += text(" · Importée");
        } else if (!path.isEmpty()) {
            progress += text(" · Prête à importer");
        }
        const QStringList values{name, folderName, QString::number(total), progress};
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values[column]);
            item->setData(Qt::UserRole, id);
            m_pTable->setItem(row, column, item);
        }
        m_pTable->setRowHidden(
                row, !QString(name + folderName).contains(m_pFilter->text(), Qt::CaseInsensitive));
        if (id == selection) {
            m_pTable->selectRow(row);
        }
    }
    updateActions();
}

void MusicdlPanel::updateActions() {
    const auto playlist = selectedPlaylist();
    const bool online = !m_snapshot.isEmpty();
    const bool idle = online && !m_snapshot.value(QStringLiteral("busy")).toBool() &&
            m_pendingCommand.isEmpty();
    m_pRefresh->setEnabled(idle && m_snapshot.value(QStringLiteral("configured")).toBool());
    m_pPrepare->setEnabled(idle && m_snapshot.value(QStringLiteral("connected")).toBool() &&
            !playlist.isEmpty() && playlist.value(QStringLiteral("track_count")).toInt() > 0 &&
            !playlist.value(QStringLiteral("preparing")).toBool());
    const QString path = playlist.value(QStringLiteral("m3uPath")).toString();
    m_pImport->setEnabled(online && !path.isEmpty() && !m_importedPaths.contains(path) &&
            QFileInfo(path).isFile() &&
            playlist.value(QStringLiteral("ready")).toInt() ==
                    playlist.value(QStringLiteral("track_count")).toInt() &&
            !playlist.value(QStringLiteral("preparing")).toBool());
}

void MusicdlPanel::sendCommand(const QString& action) {
    if (m_snapshot.isEmpty() || !m_pendingCommand.isEmpty()) {
        return;
    }
    m_pendingCommand = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_commandSentAt = QDateTime::currentMSecsSinceEpoch();
    const QJsonObject command{{QStringLiteral("version"), 1},
            {QStringLiteral("id"), m_pendingCommand},
            {QStringLiteral("sessionId"), m_snapshot.value(QStringLiteral("sessionId"))},
            {QStringLiteral("updatedAt"), static_cast<double>(m_commandSentAt)},
            {QStringLiteral("action"), action},
            {QStringLiteral("playlistId"), selectedPlaylist().value(QStringLiteral("id"))}};
    QSaveFile file(QFileInfo(requestsPath())
                    .absoluteDir()
                    .filePath(QStringLiteral("musicdl-command.json")));
    const QByteArray data = QJsonDocument(command).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) ||
            !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
            file.write(data) != data.size() || !file.commit()) {
        m_pendingCommand.clear();
        m_pFeedback->setText(text("Impossible de transmettre la demande à l’agent."));
    } else {
        m_pFeedback->setText(text("Demande envoyée à l’agent…"));
    }
    updateActions();
}
} // namespace gameflorr
