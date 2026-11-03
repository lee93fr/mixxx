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

## Fonctionnement et limites

Le thème est comparé aux métadonnées textuelles disponibles : il ne s’agit pas
d’une compréhension musicale du morceau. MusicDL ne fournit pas toujours le BPM,
la tonalité ou le genre ; Mixxx enrichit les informations après analyse. Il est
préférable de préparer et vérifier les grilles et les repères avant une soirée.
Cette version n’effectue pas de séparation de stems, d’analyse des voix, de mashup
ou de scratch automatique. Le rendu audio et le matériel doivent être essayés sur
l’installation qui sera utilisée.

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
`gameflorr-autodj-command.json` et `gameflorr-conductor.json`. Il utilise des sessions,
des commandes identifiées et des acquittements. Ces fichiers ne contiennent pas
les clés API. Un seul agent doit utiliser ce dossier à la fois.

<!-- Fin du texte généré de manière autonome par un agent IA. -->
