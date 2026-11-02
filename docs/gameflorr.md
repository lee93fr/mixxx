Texte généré par Codex (IA).

# Utiliser Mixxx avec Gameflorr

Cette version expérimentale ajoute un partage local du titre en cours. Elle nécessite aussi la version de Gameflorr Agent intégrant la source Mixxx. Une installation standard de Mixxx ne fournit pas ce pont.

## Premier lancement

1. Configurez votre carte son et votre contrôleur dans les préférences habituelles de Mixxx. Vérifiez la sortie principale et la préécoute au casque avant une soirée.
2. Ouvrez **Préférences → Interface → Gameflorr (expérimental)**, cochez **Partager le titre en cours avec l’agent Gameflorr local**, puis **Appliquer**. Aucun redémarrage n’est nécessaire.
3. Dans **Gameflorr Agent → Réglages**, collez la clé du panneau Agent Serato de votre événement, choisissez **Mixxx (expérimental)** comme logiciel source et le mode **DJ**. Laissez le fichier personnalisé vide.
4. Testez la connexion à Gameflorr, puis démarrez l’agent. Le statut doit devenir **Mixxx : pont connecté**.
5. Chargez et lancez un morceau. Montez son fader et placez le crossfader de son côté. Le titre doit apparaître dans l’agent puis dans Gameflorr.

## Un parcours simple pour mixer

Commencez avec deux platines et une disposition d’interface qui vous convient dans Préférences → Interface. Chargez le titre suivant sur la platine libre, écoutez-le au casque, puis faites votre transition avec les faders. Le pont observe les réglages du mix principal ; il ne change ni la lecture, ni la synchronisation, ni les points cue.

Pour les demandes du public, configurez l’API musique dans l’agent. Après téléchargement, utilisez **Ouvrir le dossier des titres à charger**, puis glissez le fichier terminé dans une platine Mixxx. L’agent n’écrit pas dans la base de bibliothèque Mixxx.

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

## Limites et prochaines améliorations

Cette intégration ne fournit pas encore les commandes Gameflorr vers Mixxx, l’indexation de sa bibliothèque, l’installation automatique sur Windows/macOS, ni une refonte complète de l’interface comparable à Serato. Les étapes suivantes sont un assistant de configuration audio/contrôleur, un parcours à deux platines avec progressive découverte des fonctions, puis une file de demandes intégrée à la bibliothèque. Leur validation nécessite des essais sur du matériel DJ réel. Les tests automatisés ne remplacent pas cet essai, prévu après la fusion par le propriétaire du fork.

Fin du texte généré par Codex (IA).
