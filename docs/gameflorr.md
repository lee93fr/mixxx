Texte généré par Codex (IA).

# Utiliser Mixxx avec Gameflorr

Cette version expérimentale ajoute un partage local du titre en cours, un assistant de configuration et un panneau de demandes du public. Elle nécessite aussi la version de Gameflorr Agent intégrant la source Mixxx. Une installation standard de Mixxx ne fournit pas ce pont.

## Premier lancement

1. Ouvrez **Gameflorr → Assistant de configuration…** (proposé au premier lancement du thème Gameflorr). Il guide les réglages audio, le contrôleur et le partage. Les boutons ouvrent les préférences existantes ; fermez-les pour revenir à la même étape. Vérifiez physiquement les enceintes et le casque. Le bouton Terminer nécessite les trois confirmations de test ; Plus tard ferme sans déclarer les essais réalisés.
2. Ouvrez **Préférences → Interface → Gameflorr (expérimental)**, cochez **Partager le titre en cours avec l’agent Gameflorr local**, puis **Appliquer**. Aucun redémarrage n’est nécessaire.
3. Dans **Gameflorr Agent → Réglages**, collez la clé du panneau Agent Serato de votre événement, choisissez **Mixxx (expérimental)** comme logiciel source et le mode **DJ**. Laissez le fichier personnalisé vide.
4. Testez la connexion à Gameflorr, puis démarrez l’agent. Le statut doit devenir **Mixxx : pont connecté**.
5. Chargez et lancez un morceau. Montez son fader et placez le crossfader de son côté. Le titre doit apparaître dans l’agent puis dans Gameflorr.

## Un parcours simple pour mixer

Choisissez le skin **Gameflorr** dans Préférences → Interface pour le nouveau parcours à deux platines. Le [guide de l’interface](gameflorr-interface.md) détaille les commandes et les panneaux à la demande. Chargez le titre suivant sur la platine libre, écoutez-le au casque, puis faites votre transition avec les faders. Le pont observe les réglages du mix principal ; il ne change ni la lecture, ni la synchronisation, ni les points cue.

## Demandes du public dans Mixxx

Ouvrez **Gameflorr → Demandes du public**. Le panneau affiche les demandes reçues par l’agent, y compris sans API musique. Filtrez par titre ou artiste, puis sélectionnez une ligne. Sur un écran trop bas pour afficher les platines et le panneau ensemble, les demandes s’ouvrent dans une fenêtre flottante déplaçable.

- **Chercher dans la bibliothèque** ouvre une recherche du titre dans la collection Mixxx. Vous choisissez ensuite le morceau correspondant ; aucune correspondance automatique n’est supposée.
- **Télécharger / réessayer** transmet une commande à l’agent connecté. L’API musique doit être configurée dans cet agent. Si le téléchargement automatique est désactivé, cette commande permet de lancer une demande manuellement. Les états Recherche, Téléchargement, Prêt ou À vérifier reflètent les résultats de l’agent ; le détail d’une erreur apparaît au survol de l’état.
- **Charger sur 1 / 2** charge un fichier terminé et présent, sans lancer la lecture. Une platine en lecture ou en passthrough est protégée. Les contrôles sont revérifiés au clic. Une demande sans fichier local peut toujours être recherchée dans la collection.
- Si l’agent s’arrête ou ne publie plus depuis cinq secondes, les actions de téléchargement et de chargement sont désactivées. Le dernier affichage reste visible pour information. Une reconnexion conserve les fichiers déjà prêts pour les demandes encore en attente côté serveur.

L’ancien parcours **Ouvrir le dossier des titres à charger** reste disponible dans l’agent. Le panneau natif n’effectue aucun lancement automatique de morceau. Charger un fichier utilise le parcours habituel de la bibliothèque Mixxx.

## Comportement du partage

- Le partage est désactivé par défaut. L’agent et Mixxx doivent utiliser le même compte sur le même ordinateur.
- Une fois par seconde, Mixxx remplace atomiquement `~/.gameflorr/mixxx-state.json`. Le fichier contient les métadonnées des platines, leur état de lecture et leur contribution au mix principal, sans chemin vers les fichiers audio.
- Le titre est publié lorsqu’une seule platine est en lecture, routée vers le mix principal, avec un gain de fader/crossfader supérieur à 0,001 et sans mute ni passthrough. Ce signal décrit le routage logiciel, pas une mesure du son réellement entendu.
- Si plusieurs platines contribuent, si elles sont en pause, ou si le pont s’arrête, aucun nouveau titre n’est envoyé. Gameflorr conserve le dernier titre publié. Un fichier vieux de plus de cinq secondes est ignoré par l’agent.
- Une table de mixage externe, les effets, le volume général et la préécoute peuvent produire un résultat différent de cette estimation. Vérifiez votre configuration avant utilisation publique.
- Décochez l’option puis appliquez pour arrêter les mises à jour. L’agent détecte l’arrêt après cinq secondes. Le dernier fichier local reste présent.

## Diagnostic

Si le pont reste déconnecté, vérifiez que le partage est activé dans cette version de Mixxx, que l’agent utilise la source Mixxx en mode DJ, et que les deux applications affichent/utilisent le même fichier. Le chemin affiché dans Mixxx est copiable. Un chemin personnalisé doit être absolu et accessible en écriture.

Pour un déploiement avancé, `GAMEFLORR_MIXXX_STATE_PATH` définit le fichier et force l’activation au démarrage de Mixxx ; l’option devient alors non modifiable dans l’interface. Indiquez le même chemin dans l’agent. Retirez cette variable puis relancez Mixxx pour retrouver le contrôle par la case à cocher.

## Protocole pour l’agent

Un objet JSON de version `2` contient `sessionId` (UUID du lancement), `updatedAt` (millisecondes Unix), `singleOnAirDeck` (numéro à partir de 1, ou 0 si ambigu), et `decks`. Chaque platine expose `deck`, `playing`, `onAir`, `title`, `artist`, `bpm`, `durationMs` et `position` (position normalisée). L’agent refuse les versions inconnues, les états périmés, et les sélections ambiguës. Le fichier est privé au compte utilisateur lorsque la plateforme gère ces permissions.

## Pont local des demandes

L’agent publie une fois par seconde `gameflorr-requests.json`, à côté du fichier de titre choisi : version `1`, UUID `sessionId`, `updatedAt`, `online`, `backendConnected`, `downloadsEnabled`, `lastCommand` et `requests`. Chaque ligne contient `id`, `title`, `artist`, `stage`, `filePath`, `error`. La file est bornée à 500 demandes. Les états connus sont `queued`, `searching`, `downloading`, `ready`, `skipped`, `error`. Mixxx refuse les états inconnus, les doublons et les fichiers dépassant 2 Mio.

Pour demander un téléchargement, Mixxx remplace atomiquement `gameflorr-command.json` : version `1`, même `sessionId`, UUID de commande `id`, `updatedAt`, `action: "download"`, `requestId`. L’agent exige une demande connue, une connexion validée et l’API configurée. Il ignore une autre session, une commande vieille de plus de dix secondes ou datée de plus de deux secondes dans le futur. Les commandes récentes déjà traitées ne sont pas exécutées deux fois. L’acquittement apparaît dans `lastCommand` ; Mixxx attend au plus dix secondes. Aucune commande de lecture ou chemin de fichier arbitraire n’est accepté.

Ces fichiers restent sur le compte utilisateur et ne contiennent pas les clés d’authentification de configuration. Les écritures sont atomiques et privées sur les plateformes gérant les permissions POSIX. Utilisez un seul agent pour un fichier d’état donné. Les fichiers prêts ne sont conservés en mémoire que pendant la session de l’agent ; après redémarrage, une nouvelle demande de téléchargement peut retrouver le fichier existant.

## Limites et prochaines améliorations

L’indexation de la bibliothèque vers l’agent, les commandes de transport à distance et l’installation automatique sur Windows/macOS restent à développer. L’assistant ne valide pas automatiquement le son ou le matériel réellement branché. Le mapping RANE ONE reste expérimental, sans pilotage des moteurs ni scratch. Les tests automatisés ne remplacent pas un essai sur une installation DJ réelle, prévu après la fusion par le propriétaire du fork.

Fin du texte généré par Codex (IA).
