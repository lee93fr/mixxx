<!-- Texte généré par Codex (IA). -->

# Interface Gameflorr

Gameflorr est un skin natif à deux platines visibles, dérivé de Deere. La vue Extended rapproche la disposition de Serato DJ Pro : platines compactes en haut, cadrans de tempo circulaires, formes d’onde horizontales parallèles, huit hotcues par platine et grande bibliothèque en bas. Le moteur audio, les raccourcis et les mappings restent ceux de Mixxx.

![Vue Extended Gameflorr](../res/skins/Gameflorr/skin_preview.png)

## Choisir le skin

Dans **Préférences → Interface**, choisissez **Gameflorr** et appliquez. Une nouvelle installation de ce fork le sélectionne par défaut ; un skin déjà choisi reste sélectionné. Utilisez une fenêtre d’au moins **1280 × 720**. L’aide **?** est masquée au premier lancement ; une préférence existante est conservée.

## Charger, écouter, mixer

1. Recherchez un morceau au-dessus de la bibliothèque. Sélectionnez sa ligne, puis cliquez sur **Charger** sur la platine 1 ou 2. Le glisser-déposer reste disponible.
2. Activez **PFL** sur la platine à préparer. Réglez **Casque** et **Mix** dans la barre supérieure. Configurez la sortie casque dans **Préférences → Matériel sonore**.
3. Utilisez **CUE**, **▶ / Ⅱ** et **SYNC**. **KEY** conserve la tonalité quand le tempo change. Le curseur sous le cadran ajuste le tempo ; les touches **− / +** le ralentissent ou l’accélèrent pendant le maintien. Le cadran affiche le BPM courant, la variation de tempo en pourcentage et le temps. Cliquez sur le temps pour alterner entre temps écoulé, temps restant et les deux, selon les préférences natives de Mixxx. C’est un affichage de tempo, sans animation de rotation ni commande de scratch ; les formes d’onde conservent leurs interactions natives.
4. Les huit pads créent ou rappellent les hotcues selon le comportement Mixxx. Ils reprennent la couleur des repères dans les formes d’onde. Le clic droit permet de modifier leur couleur ; la coloration automatique des nouveaux hotcues se règle dans **Préférences → Couleurs**. Les boutons **1 / 2 / 4 / 8** activent ou quittent une boucle de ce nombre de temps.
5. Les deux faders, les vumètres et le crossfader restent au centre dans la vue compacte. **Mixeur** affiche les gains, égaliseurs et filtres natifs. **Master** règle la sortie principale.

## Afficher davantage de commandes

- **Détails** : grille rythmique, réglages de boucles et sauts.
- **FX** : unités d’effets Mixxx.
- **Sampler** : lecteurs de samples Mixxx.
- **Mixeur** : égaliseurs et mixeur complet ; cliquez à nouveau pour revenir au centre compact.
- **Bibliothèque** : collection agrandie avec transports compacts ; cliquez à nouveau pour revenir à Extended.
- **?** : aide de démarrage et rappel des réglages audio.

Détails, FX et Sampler s’affichent un à la fois. Cliquez sur le panneau actif pour le fermer. L’aide, le mixeur et le dernier panneau avancé sont mémorisés. Masquer un effet ou un sampler ne coupe pas son son. Sur un petit écran, affichez les panneaux selon vos besoins.

Le skin affiche uniquement les platines 1 et 2. Les platines 3 et 4 restent disponibles au moteur pour la compatibilité avec les composants Deere : un contrôleur peut encore les piloter. Choisissez un autre skin pour mixer visuellement à quatre platines.

## Partage Gameflorr

Le skin et le partage de titres se règlent séparément. Dans **Préférences → Interface → Gameflorr (expérimental)**, activez le partage puis choisissez la source Mixxx et le mode DJ dans l’agent. Consultez le [guide du pont](gameflorr.md) pour le diagnostic et les demandes du public.

## Périmètre et maintenance

La disposition s’inspire du [mode Extended de Serato DJ Pro](https://support.serato.com/hc/en-us/articles/225122768-Display-Modes). Les graphismes des cadrans sont propres à Gameflorr. Le skin ne fournit pas d’assistant audio/contrôleur ni de file de demandes intégrée. L’essai de préécoute, de contrôleur et de transition sur votre matériel reste à faire avant une soirée.

Les petits fichiers de redirection XML permettent aux références `skin:` internes des composants Deere de se résoudre dans Gameflorr. Ils réutilisent les composants natifs sans copier leur logique. Si un composant Deere ajoute une dépendance `skin:`, ajoutez sa redirection et vérifiez les panneaux concernés dans Mixxx.

Deere est créé par RJ Ryan et S.Brandt sous licence Creative Commons Attribution, Share-Alike 3.0 Unported. Les composants réutilisés et les adaptations Gameflorr conservent cette licence.

Fin du texte généré par Codex (IA).
