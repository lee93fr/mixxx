> Documentation générée par un agent IA, à relire et valider par un développeur humain.

# Cohabitation avec une bibliothèque Serato

Cette évolution permet d'utiliser les mêmes fichiers audio dans Mixxx et Serato et de consulter des métadonnées communes. Les bases des deux applications restent indépendantes.

## Utilisation

1. Dans la bibliothèque Mixxx, faire un clic droit sur les en-têtes et choisir **Serato library columns**. Le choix est enregistré pour cette vue. Les colonnes restent réordonnables et masquables individuellement.
2. Dans les propriétés d'un morceau, les champs **Label** et **Remixer** permettent de consulter et modifier le label et le remixeur. Ils sont aussi modifiables dans la bibliothèque et recherchables avec `label:` et `remixer:`.
3. Dans la rubrique **Serato**, cliquer sur **Add a Serato library folder**, puis sélectionner `_Serato_` ou son dossier parent. Le dossier doit contenir `database V2`. Plusieurs emplacements peuvent être ajoutés et sont conservés au redémarrage. Le lien d'actualisation relit les bibliothèques après une modification dans Serato.
4. Ouvrir une bibliothèque ou une crate, puis charger un morceau normalement. Mixxx utilise le fichier à son emplacement existant.

Le préréglage affiche les colonnes disponibles dans chaque source : titre, artiste, BPM, tonalité, durée, commentaire, regroupement, genre, album, label, remixeur, compositeur, année, numéro de piste, notation, compteur de lectures, date d'ajout, débit, fréquence d'échantillonnage, format, nom de fichier, taille et emplacement. Les sources externes peuvent exposer moins de colonnes que la bibliothèque Mixxx.

## Métadonnées et migration

Les champs label et remixeur sont désormais enregistrés dans la base Mixxx, y compris dans une compilation standard. Les bibliothèques existantes passent du schéma 40 au schéma 41 sans déplacer les fichiers audio. Les champs initialement absents sont complétés à la lecture des tags ; les valeurs déjà présentes et les champs explicitement vidés après édition sont conservés.

La lecture et l'écriture utilisent les formats de tags déjà pris en charge par Mixxx : ID3v2 (`TPUB`, `TPE4`), Vorbis comments (`LABEL`, `REMIXER`), MP4 (champs iTunes `LABEL`, `REMIXER`) et APE (`Label`, `MixArtist`). L'écriture dans les fichiers suit les préférences et actions d'export de métadonnées existantes de Mixxx.

La rubrique Serato ouvre `database V2` et les crates en lecture seule. Elle expose désormais le label et la date d'ajout déjà présents dans ces données. Les nouveaux champs ne constituent pas une synchronisation automatique entre les bases.

## Limites

- Pas de synchronisation bidirectionnelle des crates ou des modifications de métadonnées.
- Pas de nouvelle prise en charge des Smart Crates, Flips ou données vidéo.
- Les statistiques, notations et dates d'ajout de Mixxx restent propres à sa bibliothèque ; elles ne sont pas fusionnées avec celles de Serato.
- La compatibilité avec une bibliothèque Serato réelle et le retour des tags dans Serato doivent encore être validés manuellement. Les tests automatisés vérifient Mixxx et des fichiers de démonstration.

## Références

- [Serato — colonnes de bibliothèque](https://support.serato.com/hc/en-us/articles/203015464-Sorting-and-browsing-your-library)
- [Mixxx — bibliothèque Serato](https://manual.mixxx.org/2.5/en/chapters/library#using-the-serato-library)

> Fin de la documentation générée par un agent IA.
