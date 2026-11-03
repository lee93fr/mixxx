<!-- Texte généré par Codex (IA). -->

# Interface Gameflorr

Gameflorr est un skin natif à deux platines visibles, dérivé de Deere. La vue **Horizontal**, sélectionnée par défaut, place les platines aux extrémités et les formes d’onde au centre. **Extended** utilise toute la largeur pour les formes d’onde, avec les cadrans sur les bords. Les cadrans sombres gardent le repère tournant natif. Le moteur audio, les raccourcis et les mappings restent ceux de Mixxx.

![Vue Horizontal Gameflorr](../res/skins/Gameflorr/skin_preview.png)

## Choisir le skin

Dans **Préférences → Interface**, choisissez **Gameflorr** et appliquez. Une nouvelle installation de ce fork le sélectionne par défaut ; un skin déjà choisi reste sélectionné. Utilisez une fenêtre d’au moins **1280 × 720**. L’aide **?** est masquée au premier lancement ; une préférence existante est conservée.

## Charger, écouter, mixer

Le master tempo (**KEY**) est activé automatiquement au démarrage sur les platines, y compris si un ancien profil l’avait désactivé. Vous pouvez le désactiver pendant la session ; charger un autre morceau ou changer de vue conserve votre choix. Il sera réactivé au prochain démarrage. Les samplers et la préécoute de bibliothèque gardent leur réglage habituel.

1. Recherchez un morceau au-dessus de la bibliothèque. Sélectionnez sa ligne, puis cliquez sur **Charger** sur la platine 1 ou 2. Le glisser-déposer reste disponible.
2. Activez **PFL** sur la platine à préparer. Réglez **Casque** et **Mix** dans la barre supérieure. Configurez la sortie casque dans **Préférences → Matériel sonore**.
3. Utilisez **CUE**, **▶ / Ⅱ** et **SYNC**. **KEY** conserve la tonalité quand le tempo change. Le curseur sous le cadran ajuste le tempo ; les touches **− / +** le ralentissent ou l’accélèrent pendant le maintien. Le cadran affiche le BPM courant, la variation de tempo en pourcentage et le temps. Sa barre bleue tourne avec la position de lecture : elle s’immobilise en pause, suit le pitch, les boucles et les sauts vers les hotcues. Sa vitesse de référence est de 33⅓ tours/minute, ou celle du vinyle configuré dans Mixxx. Cliquez sur le temps pour alterner entre temps écoulé, temps restant et les deux, selon les préférences natives de Mixxx. Le cadran n’ajoute pas de commande de scratch ; les formes d’onde conservent leurs interactions natives.
4. Choisissez le mode au-dessus des huit pads : **Hot Cue**, **Roll**, **Slicer**, **Sampler**, **Loop**, **Jump** ou **S. Loop**. Le mode est propre à chaque platine. Les pads à l’écran et ceux des mappings RANE ONE/DDJ-1000 de ce fork partagent la même commande. Le changement de mode libère les appuis momentanés.
5. En vue Horizontal, **Mixeur** affiche les faders, les gains, les égaliseurs et les filtres natifs sous les platines. En vue Extended, les deux faders, les vumètres et le crossfader restent au centre en mode compact. **Master** règle la sortie principale.

## Jouer avec les pads

| Mode | Pads 1 à 8 | Clic droit / Shift+pad sur le contrôleur |
| --- | --- | --- |
| Hot Cue | Créer ou rappeler les repères 1–8 | Effacer le repère |
| Roll | Boucles momentanées de ⅛, ¼, ½, 1, 2, 4, 8 et 16 temps | — |
| Slicer | Huit tranches d’un temps dans une zone qui avance avec le morceau | — |
| Sampler | Relancer les samples 1–8 depuis leur cue | Arrêter le sample |
| Loop | Activer/quitter une boucle de ¼, ½, 1, 2, 4, 8, 16 ou 32 temps | Quitter la boucle |
| Jump | Sauts −1/+1, −2/+2, −4/+4, −8/+8 temps | — |
| S. Loop | Conserver une zone de huit temps et jouer ses tranches | — |

Le Roll et le Slicer utilisent le Slip natif : la progression continue virtuellement pendant l’appui. Le Slicer restaure les points de boucle et le réglage Slip précédents au relâchement. Si plusieurs tranches sont maintenues, la dernière appuyée joue ; relâchez-la pour revenir à la précédente. S. Loop conserve sa zone jusqu’au changement de mode et restaure ensuite la boucle précédente.

Le Slicer exige une grille de beats et une zone complète de huit temps dans le morceau. Le message « Vérifier la grille et la position » signale son indisponibilité ; vérifiez aussi la position près du début ou de la fin du morceau. Corrigez la grille avant de l’utiliser. Les modes reviennent à Hot Cue au démarrage. Les huit samples sont partagés entre les platines et restent chargés lorsque le panneau est masqué. Ce lot ne fournit pas de séparation de stems.

Les modes requièrent le nouveau binaire de ce fork. La RANE ONE utilise **Parameter −/+** pour parcourir les modes à l’écran ; ses boutons matériels de sélection de mode ne sont pas encore décodés. Le DDJ-1000 utilise ses boutons de mode, avec les correspondances détaillées dans son [guide](../res/controllers/Pioneer-DDJ-1000.md). Les deux mappings restent à tester sur matériel réel.

## Organiser l’écran et la bibliothèque

- **Horizontal** : platines latérales de largeur fixe et formes d’onde centrales qui suivent la taille de la fenêtre.
- **Extended** : platines compactes en haut et formes d’onde sur toute la largeur.
- **Bibliothèque** : collection agrandie avec des transports compacts. Cliquez à nouveau ou choisissez Horizontal/Extended pour revenir au mixage. Le raccourci natif d’agrandissement reste disponible.

Déplacez la séparation au-dessus de la bibliothèque pour régler la hauteur des formes d’onde. La séparation et le mode de mixage sont mémorisés. Les vues utilisent les mêmes morceaux et commandes ; changer de vue ne coupe pas la lecture.

La zone avant le début du morceau reste vide, sans triangles bleus. Le skin utilise l’option native `ShowPreroll` de ce fork, désactivée dans Gameflorr ; les indicateurs après la fin du morceau restent affichés.

Sur une nouvelle bibliothèque, les colonnes sont : préécoute, titre, artiste, BPM, tonalité, durée, note et genre. Les listes ordonnées conservent aussi leur colonne de position. Les dispositions déjà enregistrées sont conservées. Pour appliquer le préréglage, faites un clic droit sur l’en-tête puis choisissez **Colonnes DJ : titre, artiste, BPM, tonalité**. Vous pouvez ensuite déplacer, masquer et redimensionner les colonnes normalement. Le préréglage est réservé au skin Gameflorr.

La recherche occupe la largeur disponible. Les boutons **Charger 1** et **Charger 2** au-dessus de la table chargent le morceau sélectionné sur la platine correspondante, en respectant la protection de chargement native de Mixxx.

## Scanner et analyser la bibliothèque

Le bouton **Scanner la bibliothèque**, au-dessus des morceaux, recherche les changements dans les dossiers musicaux configurés, puis analyse les titres dont le BPM ou la tonalité manque. Les dossiers se choisissent dans **Préférences → Bibliothèque**.

La même barre affiche les éléments traités pendant la recherche, puis le nombre de titres et le pourcentage pendant l’analyse BPM/tonalité. **Annuler** arrête le travail ; les états terminé, annulé ou interrompu restent affichés dans la bibliothèque. Le fichier en cours est indiqué au survol du statut. Le scan et l’analyse par lots utilisent tous les cœurs détectés ; l’analyse au chargement d’un morceau utilise également cette capacité.

## Dispositions locales conservées

- **Vertical** : deux formes d’onde verticales côte à côte avec leurs vumètres. Les faders, le crossfader et le bouton Mixeur sont masqués dans cette vue.
- **Compacte** : masque les cadrans, conserve BPM, temps, pitch et PFL et réduit les formes d’onde horizontales à 64 pixels de hauteur. Les modes de pads et les transports restent disponibles.

De petits numéros gris indiquent les débuts de mesure, par groupes de quatre temps, selon la grille rythmique. Ils restent droits en vue verticale et sont espacés pour éviter les chevauchements. Sans grille, aucun numéro ne s’affiche.

## Afficher davantage de commandes

- **Détails** : grille rythmique, réglages de boucles et sauts.
- **FX** : unités d’effets Mixxx.
- **Sampler** : huit lecteurs de samples avec chargement par glisser-déposer, lecture, arrêt, répétition et volume individuel.
- **Mixeur** : égaliseurs et mixeur complet ; cliquez à nouveau pour le masquer ou revenir au centre compact en vue Extended.
- **?** : aide de démarrage et rappel des réglages audio.

Détails et FX s’affichent un à la fois. Le Sampler dispose de son propre bouton et peut rester ouvert avec les effets. Cliquez sur le bouton actif pour masquer son panneau. Le passage au mode de pads Sampler ouvre le panneau ; vous pouvez ensuite le masquer sans changer de mode. L’aide, le mixeur et la visibilité des panneaux sont mémorisés. Les fonctions Gameflorr restent accessibles par leur menu natif, sans occuper en permanence l’espace de mixage. Masquer un effet ou un sampler ne coupe pas son son. Sur un petit écran, affichez les panneaux selon vos besoins.

Le skin affiche uniquement les platines 1 et 2. Les platines 3 et 4 restent disponibles au moteur pour la compatibilité avec les composants Deere : un contrôleur peut encore les piloter. Choisissez un autre skin pour mixer visuellement à quatre platines.

## Partage Gameflorr

Le skin et le partage de titres se règlent séparément. Dans **Préférences → Interface → Gameflorr (expérimental)**, activez le partage puis choisissez la source Mixxx et le mode DJ dans l’agent. Consultez le [guide du pont](gameflorr.md) pour le diagnostic et les demandes du public.

## Périmètre et maintenance

La disposition s’inspire des [modes d’affichage de Serato DJ Pro](https://support.serato.com/hc/en-us/articles/225122768-Display-Modes). Les graphismes des cadrans sont propres à Gameflorr. Les demandes Gameflorr, le réglage des clés API et le choix de l’écran diffusé restent dans les dialogues natifs existants. Cette refonte ne fournit pas de file Prepare intégrée à la bibliothèque. L’essai de préécoute, de contrôleur et de transition sur votre matériel reste à faire avant une soirée.

Les petits fichiers de redirection XML permettent aux références `skin:` internes des composants Deere de se résoudre dans Gameflorr. Ils réutilisent les composants natifs sans copier leur logique. Si un composant Deere ajoute une dépendance `skin:`, ajoutez sa redirection et vérifiez les panneaux concernés dans Mixxx.

Le composant natif `SpinnyIndicator` peint le repère en utilisant la position audio interpolée de Mixxx et le rafraîchissement des formes d’onde. Il laisse les événements souris aux commandes existantes. Cette version du skin nécessite le binaire de ce fork qui fournit ce composant.

Deere est créé par RJ Ryan et S.Brandt sous licence Creative Commons Attribution, Share-Alike 3.0 Unported. Les composants réutilisés et les adaptations Gameflorr conservent cette licence.

Fin du texte généré par Codex (IA).
