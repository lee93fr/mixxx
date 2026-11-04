<!-- Texte généré de manière autonome par un agent IA. -->

# Conducteur AutoDJ Gameflorr et MusicDL

Le conducteur sélectionne les morceaux dans les playlists MusicDL et les demandes
approuvées de l’événement. Mixxx assure la lecture et les transitions avec son moteur
AutoDJ natif. Il nécessite les versions correspondantes de Mixxx, du backend
Gameflorr, de la console DJ et de Gameflorr Agent, sur le même ordinateur pour
Mixxx et l’agent. Aucune migration de base de données n’est nécessaire.

## Mise en route

1. Configurez la clé de l’événement et la connexion MusicDL dans Gameflorr Agent.
   Choisissez la source **Mixxx** et le mode **Autonome**, puis démarrez l’agent.
2. Ouvrez Mixxx et configurez ses sorties audio. Si vous utilisez un chemin d’état
   personnalisé, choisissez le même dans les deux applications.
3. Dans Gameflorr, sélectionnez la sortie **Agent**, puis ouvrez **AutoDJ** pour
   l’événement concerné. Sélectionnez les playlists MusicDL à utiliser.
4. Réglez le thème, l’énergie, la place des demandes, les artistes exclus et, au
   besoin, des créneaux horaires avec leurs playlists et un titre imposé.
   Les horaires de la console sont affichés dans le fuseau local du navigateur.
5. Sauvegardez le programme, vérifiez la suite proposée et lancez le conducteur.
   Le même programme est accessible dans l’onglet **Conducteur AutoDJ** du panneau
   Gameflorr de Mixxx. L’édition native des créneaux utilise des dates ISO avec
   fuseau horaire, par exemple `2026-11-04T20:00:00+01:00`.

L’agent conserve le programme et les 200 dernières lectures pour l’événement.
Le serveur conserve également le programme et l’historique réel. Une reprise de
connexion restaure le programme ; elle ne lance pas automatiquement le son.

## Modes et commandes

- **Assisté** prépare la suite. **Lancer / autoriser** autorise une transition,
  puis le conducteur rend la main après l’enchaînement.
- **Automatique sobre** utilise les transitions natives avec un fondu simple.
- **Automatique créatif** choisit des transitions de 8, 16 ou 32 mesures selon
  la créativité et la durée disponible entre les repères intro/outro. Si les
  grilles sont stables et les tempos proches, il synchronise le tempo et la phase,
  active le verrouillage de tonalité sur la platine entrante et échange les basses.
  Une analyse insuffisante, une plage trop courte ou un écart de tempo important
  déclenche un fondu plus simple.
- **Enchaîner maintenant** demande un enchaînement natif vers le prochain titre prêt.
- **Verrouiller** donne priorité à un titre de la suite proposée, sous réserve des
  exclusions et de l’anti-répétition.
- **Reprendre la main** désactive le pilotage automatique en conservant la lecture
  et le crossfader. Un réglage de basses modifié par le DJ est préservé.

Le sélecteur tient compte des demandes approuvées et de leurs votes, du thème,
de l’énergie, des BPM et tonalités connus et des titres/artistes récemment joués.
Chaque proposition affiche sa raison et son état de préparation. Les métadonnées
des playlists sont chargées sans télécharger leur totalité ; lorsque le conducteur
est actif, l’agent prépare les cinq prochaines propositions. Un titre prêt peut
prendre le relais pendant la préparation des autres.

## Quatre stems et mashups vocaux

Dans les réglages de l’agent, installez le moteur **Instrumentaux** (Mac M1+ avec
Metal ou Windows NVIDIA CUDA). Dans le conducteur, activez **Utiliser les 4 stems
préparés** et sauvegardez. Lorsque le conducteur est actif, l’agent prépare les
deux prochaines propositions, une à la fois. Le bouton **Préparer 4 stems** permet
aussi de préparer explicitement un titre local avant le démarrage. L’avancement,
les erreurs et l’annulation restent accessibles dans Gameflorr et Mixxx.

Les fichiers source sont conservés. Un cache privé contient les conteneurs
`.stem.m4a` : batterie, basse, instruments et voix, plus un flux du mix complet.
La séparation travaille en arrière-plan ; le moteur audio ne calcule pas le modèle.
Un résultat prêt sera utilisé lors d’un prochain chargement. Il ne remplace pas
une piste déjà en lecture. Le bouton **Stems** du skin affiche ou masque les quatre
commandes MUTE pour les pistes qui contiennent des stems.

Activez ensuite **Mashup vocal en mode créatif**, choisissez **Automatique créatif**
et sauvegardez. Si les deux pistes ont les quatre stems attendus, la même tonalité
connue, des grilles stables, des tempos proches (écart maximal de 6 %) et au moins
huit mesures disponibles, Mixxx retire l’accompagnement sortant et les voix entrantes,
conserve la voix sortante sur l’instrumental entrant, puis passe à la voix entrante.
Les autres cas conservent la transition classique. Ce comportement ne détecte pas
les phrases chantées : vérifiez les grilles et les repères intro/outro pour obtenir
un résultat musical. Il ne produit pas de fichier remixé exportable.

Une modification manuelle du gain ou du mute d’un stem arrête l’automation des
stems et préserve la commande du DJ ; le fondu continue. **Reprendre la main**
désactive le conducteur. Les gains contrôlés par l’automation sont restaurés.
**Préécouter au casque** joue le titre complet sur la platine de préécoute de Mixxx,
sans démarrer AutoDJ ni déclarer le titre joué. Cette commande ne simule pas le
mashup ; configurez d’abord une sortie casque distincte du mix principal.

## Mashup live sur les platines A et B

Les quatre boutons **Voix A + instru B**, **Voix B + instru A**,
**Passer la voix** et **Mix complet** sont accessibles dans le skin Gameflorr
(affichage Stems), le panneau Conducteur AutoDJ et la console DJ de Gameflorr.
Les commandes natives fonctionnent aussi sans connexion à l’agent.

1. Préparez les stems, puis chargez les conteneurs sur les platines 1 (A) et 2 (B).
2. Reprenez la main sur AutoDJ. Vérifiez les grilles, la même tonalité connue,
   le verrouillage de tonalité sur les deux platines et les tempos synchronisés
   (écart maximal 0,5 %). Tous les stems doivent être sans mute, avec des gains
   compris entre 0 et 1.
3. Démarrez les deux pistes sur les phrases choisies et réglez les volumes et
   le crossfader pour entendre les deux platines. Le mashup live ne les déplace pas.
4. Choisissez la voix A ou B. Le mashup maintient cette voix sur l’instrumental
   de l’autre platine jusqu’à une autre commande. **Passer la voix** inverse les
   rôles. Chaque changement applique un fondu de 250 ms, sans augmenter les gains.
5. **Mix complet** restitue progressivement les gains présents avant le mashup.

Un gain ou mute modifié à la main arrête le mashup et conserve la modification.
Un arrêt de platine, un changement de tempo incompatible ou de tonalité déclenche
également la restitution des gains. Si une piste est remplacée, la restitution
ne touche jamais les gains de sa remplaçante. AutoDJ doit rester désactivé pendant
ce mode. Les commandes à distance expirent après cinq secondes et sont liées
aux pistes chargées pour éviter leur application à une autre paire de titres.

Le statut explique les conditions manquantes. L’alignement des phrases et des
voix reste au DJ : ce mode ne détecte pas les passages chantés et ne simule pas
la transition dans la préécoute casque.

## Fonctionnement et limites

Le thème est comparé aux métadonnées textuelles disponibles : il ne s’agit pas
d’une compréhension musicale du morceau. MusicDL ne fournit pas toujours le BPM,
la tonalité ou le genre ; Mixxx enrichit les informations après analyse. Il est
préférable de préparer et vérifier les grilles et les repères avant une soirée.
La préparation des stems est limitée aux sources de 250 Mo et aux titres de
30 minutes maximum ; elle partage le moteur GPU de préparation des instrumentaux.
Une absence de moteur ou un échec de préparation laisse le fichier original
disponible. Cette version n’effectue pas d’analyse des phrases vocales ni de scratch
automatique. Le rendu audio et le matériel doivent être essayés sur l’installation
qui sera utilisée.

Les demandes non approuvées peuvent être préparées dans le panneau habituel, mais
ne sont pas sélectionnées par le conducteur. Une demande est déclarée jouée après
le retour de lecture de Mixxx, pas au téléchargement ou à la sélection. Pendant
un fondu, ce retour suit la platine qui contribue le plus au crossfader ; ce n’est
pas une mesure du son réellement entendu dans la salle.

Si l’agent cesse de publier pendant cinq secondes, Mixxx rend la main en conservant
le son. Un changement de sortie vers le navigateur arrête les platines gérées par
le conducteur afin d’éviter une lecture simultanée. Un AutoDJ natif déjà actif
sans être piloté par le conducteur doit être arrêté avant la prise de contrôle.

Le pont échange des fichiers privés atomiques, bornés à 2 Mio, dans le dossier
du fichier d’état Mixxx : `mixxx-autodj-plan.json`, `gameflorr-autodj-state.json`,
`gameflorr-autodj-command.json`, `gameflorr-conductor.json` et `gameflorr-stems.json`.
Il utilise des sessions,
des commandes identifiées et des acquittements. Ces fichiers ne contiennent pas
les clés API. Un seul agent doit utiliser ce dossier à la fois.

<!-- Fin du texte généré de manière autonome par un agent IA. -->
