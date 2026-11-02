<!-- Texte généré avec l’assistance d’un agent IA. -->

# Connexions et écran public Gameflorr

Le menu **Gameflorr → Paramètres et clés API…** configure l’URL Gameflorr, la clé d’agent de
l’événement, l’ID facultatif de la soirée, l’URL MusicDL et son jeton. Ouvrir l’application
Gameflorr Agent sur le même ordinateur. Les tests utilisent les valeurs saisies sans sauvegarde.
Enregistrer applique la connexion et démarre l’agent en mode DJ/Mixxx. Laisser un champ de clé
vide conserve la clé existante. Les secrets ne sont pas enregistrés dans les préférences Mixxx.

Le bouton **Écran Gameflorr…** du panneau Demandes/MusicDL ouvre le choix du contenu public.
La case correspondante dans les paramètres le masque ou le rétablit immédiatement et mémorise
le choix. Le menu **Gameflorr → Écran public Gameflorr…** reste disponible.
Un changement de sélection ne diffuse rien avant le clic explicite sur **Appliquer à l’écran public**.
La rotation réutilise sa configuration Gameflorr existante ; les jeux restent lancés depuis leur console.

Le pont local de paramètres est fourni par la version Electron de Gameflorr Agent et nécessite sa
mise à jour. Le contrôle d’écran nécessite aussi le backend disposant des routes agent
`/api/serato/display-state` et `/api/serato/display-mode`, qui vérifient la clé et l’événement avant
mutation. Aucune migration n’est nécessaire. Les fichiers de pont se trouvent à côté de l’état
Mixxx ; les clés transitent uniquement dans une commande privée temporaire, supprimée à sa lecture,
et sont conservées dans la configuration privée existante de l’agent.

<!-- Fin du texte généré avec l’assistance d’un agent IA. -->
