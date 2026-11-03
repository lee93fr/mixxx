<!-- AI-generated documentation begins. -->

# Pioneer DDJ-1000 — mapping MIDI expérimental

Ce mapping cible le **DDJ-1000 original**, avec quatre platines logicielles.
Le DDJ-1000SRT utilise un autre protocole et n’est pas pris en charge par ce preset.
Il fonctionne indépendamment du thème choisi, dont Gameflorr.
Les tests utilisent des messages MIDI simulés et la bibliothèque Components JS
réelle de Mixxx. **Aucun essai sur un DDJ-1000 physique n’a été réalisé.**

## Activation

Les fichiers sont inclus dans les ressources de ce fork. Le lanceur Gameflorr local
les utilise directement avec le nouveau binaire de ce fork. Les commandes de pads
ne sont pas disponibles dans Mixxx standard. Pour une autre installation de ce fork, copiez
`Pioneer-DDJ-1000.midi.xml` et `Pioneer-DDJ-1000-scripts.js` dans le dossier des
mappings utilisateur, accessible dans Préférences → Contrôleurs, puis relancez Mixxx.
La bibliothèque `midi-components-0.0.js` est fournie par Mixxx.

1. Branchez et allumez le DDJ-1000 ; choisissez le port USB A ou B correspondant
   sur les sélecteurs des voies.
2. Dans **Préférences → Contrôleurs**, sélectionnez le port MIDI DDJ-1000,
   choisissez **Pioneer DDJ-1000 (experimental, 4 decks)** et activez-le.
3. Configurez les sorties audio et le casque séparément dans les préférences audio.
   Installez le pilote du fabricant si votre système le nécessite. Le mapping
   n’installe aucun pilote audio ; sous Linux, la disponibilité de la carte son
   intégrée reste à vérifier sur votre système. Une interface audio séparée peut
   être nécessaire.
4. Commencez à faible volume, puis vérifiez les deux côtés et les couches 1/3 et 2/4.

L’activation demande au moins quatre platines et huit samplers à Mixxx.
Le thème peut afficher seulement deux platines ; choisissez une vue à quatre
platines pour contrôler visuellement les couches 3 et 4.
Utilisez un seul ordinateur : la gestion du routage MIDI entre deux hôtes USB
n’est pas implémentée.

## Commandes

| Contrôle | Action Mixxx |
| --- | --- |
| Play, Cue, Shift+Cue | Lecture/pause, cue, retour au début |
| Beat Sync, Shift+Beat Sync | Activer/désactiver la synchronisation, définir la platine meneuse |
| Master Tempo | Activer/désactiver le verrouillage de tonalité ; conserve le réglage par défaut de l’application |
| Shift+Master Tempo | Plages de tempo ±6 %, ±10 %, ±16 %, ±100 % |
| Tempo, Trim, EQ, faders | Commandes 14 bits, reprise douce après réception de la première position complète |
| Color par voie | Potentiomètre du Quick Effect Mixxx ; choisissez l’effet dans Mixxx |
| Cue casque, Headphones Mixing | Préécoute de la voie, mélange de préécoute |
| Crossfader et assignation A/Thru/B | Crossfader et orientation de chaque voie |
| Jog tactile en mode Vinyl | Scratch logiciel ; sensibilité expérimentale |
| Bord du jog / mode non Vinyl | Correction temporaire de vitesse |
| Shift+jog / jog+Search | Déplacement rapide |
| Search arrière/avant | Recherche pendant l’appui |
| Quantize, Slip | Activer/désactiver la quantification et le mode Slip |
| Loop In/Out | Définir les points hors boucle ; diviser/multiplier la longueur pendant une boucle |
| Shift+Loop In/Out | Point d’entrée / reloop |
| 4 Beat Loop/Exit | Boucle de quatre temps / sortie |
| Browse, Shift+Browse | Déplacement dans la bibliothèque / déplacement accéléré |
| Appui Browse | Charger dans la platine sélectionnée par le matériel ; bloqué si elle joue |
| Back, View | Reculer le focus, agrandir/réduire la bibliothèque |
| Hot Cue | Mode Hot Cue (repères 1–8) ; page 2 : repères 9–16 |
| Pad FX 1 | Roll momentané de ⅛ à 16 temps |
| Shift+Pad FX 1 : Pad FX 2 | Slicer à huit tranches |
| Shift+Hot Cue : Keyboard | Slicer Loop (zone conservée) |
| Beat Jump, page 1 | Paires arrière/avant de 1, 2, 4 et 8 temps |
| Shift+Beat Jump : Beat Loop, page 1 | Boucles de ¼, ½, 1, 2, 4, 8, 16 et 32 temps |
| Sampler | Démarrer les samplers 1–8 préchargés ; Shift+pad arrête le sampler |
| Shift+Sampler : Key Shift | Beat Jump |

Les huit pads de chaque banque suivent le mode indiqué à l’écran, y compris après
un changement à la souris. Les fonctions Keyboard, Key Shift et Pad FX du fabricant
sont remplacées par les modes ci-dessus. Le Slicer exige une grille de beats et
une zone complète de huit temps. Les samplers 1–8 sont partagés entre platines.
Voir le [guide des pads](../../docs/gameflorr-interface.md).

Les retours LED suivent les transports, le verrouillage de tonalité, la préécoute,
les hotcues et les boucles. Les hotcues utilisent une couleur fixe de la palette
matérielle ; leur rendu et le comportement des LED restent à confirmer sur place.
Les changements de couche, de mode de pads, de page et de Shift libèrent les
commandes momentanées concernées pour éviter les appuis bloqués.

## Limites et validation matérielle

Les écrans des jogs, leurs marqueurs, les vumètres, les Beat FX matériels, les
boutons de sélection Color FX, les fonctions musicales natives Pad FX/Keyboard/Key Shift,
les memory cues, Key Sync/Reset, Slip Reverse, le fader start
et Shift+Play ne sont pas intégrés. Les niveaux master, booth, casque, micro et
sampler restent à régler sur le matériel ou dans Mixxx selon le routage audio.
Le potentiomètre Headphones Mixing pilote Mixxx : vérifiez son interaction avec
le mélange matériel lors du test audio.

La résolution de scratch `PioneerDDJ1000.jogResolution` vaut provisoirement 720,
avec `bendScale = 0.1`. Ce sont des paramètres de calibration, pas une résolution
mesurée sur le DDJ-1000. Vérifiez un tour de plateau, le sens, la vitesse et le
relâchement avant d’ajuster ces valeurs dans le script. Le réglage de friction
mécanique reste propre au contrôleur.

Avant une utilisation en soirée, vérifiez le son master/casque, les quatre voies,
la protection de chargement, les extrémités et le centre des faders, les jogs,
les changements de couche pendant un appui, les pads et leurs LED. Vérifiez aussi
qu’une désactivation du mapping libère les jogs et les boutons maintenus.

## Protocole et tests

Les adresses sont fondées sur la liste constructeur
[DDJ-1000 List of MIDI message, version 1.00](https://www.pioneerdj.com/-/media/pioneerdj/software-info/controller/ddj-1000/ddj-1000_midi_message_list_e1.pdf),
également consultable dans cette
[copie du PDF Pioneer](https://device.report/m/695cab0cd33ab70508ede37358a36bdf24485a1d950e199ba9d8baffa8dd65a6.pdf).
Pages 1–3 : navigation, platines et mixage ; pages 4–8 : pads, pages et modes.
Le [travail exploratoire Mixxx sur le DDJ-1000](https://github.com/mixxxdj/mixxx/wiki/ddj-1000)
fournit le contexte du mixage interne, des effets et des deux ports USB.
Aucun script de mapping tiers n’est inclus.

Depuis la racine du dépôt :

```sh
node --test tools/test_ddj_1000_mapping.cjs tools/test_rane_one_mapping.cjs
```

Les tests contrôlent le routage XML/JavaScript, les quatre platines, les messages
Note Off avec vélocité, les valeurs 14 bits et leur ordre, les jogs, les hotcues,
les pads, les boucles, le chargement protégé et le nettoyage à l’arrêt.
Ils ne valident ni le pilote audio ni les propriétés physiques du contrôleur.

<!-- AI-generated documentation ends. -->
