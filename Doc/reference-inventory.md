# Identifier les références historiques

Travail de la [story #20](https://github.com/boisdal/SpaceCadetPinball/issues/20).
Le catalogue `reference-fingerprints.json` conserve les empreintes documentaires
et observées avec leur provenance. Une correspondance reconnaît un fichier,
pas l'authenticité d'une installation entière. SHA-1 permet la comparaison avec
les sources historiques ; les nouvelles observations calculent aussi SHA-256.

## Inventaire reproductible

Depuis la racine du dépôt, avec Python 3.10 ou supérieur, sans dépendance :

```powershell
python tools/reference_inventory.py C:/mes-ressources/Cadet --layout full-tilt > cadet-inventory.json
python tools/reference_inventory.py C:/mes-ressources/Pinball --layout windows > xp-inventory.json
python -m unittest discover -s tools -p 'test_*.py'
```

Choisir le dossier contenant directement le DAT. Conserver les rapports hors du
dossier analysé. L'outil ne lance aucun exécutable, ne modifie aucune ressource et
émet seulement chemins relatifs, tailles, empreintes et diagnostics. Le paramètre
`--layout` indique l'organisation attendue, jamais une édition détectée. Il lit
les groupes sonores du DAT selon `partman.cpp`, `partman.h` et `loader.cpp` : les
noms WAV déclarés viennent des groupes de type 202, pas d'une liste supposée.
Tous ces groupes ne sont pas nécessairement utilisés pendant une partie : cet
inventaire statique ne calcule pas les dépendances effectivement sollicitées.

Les WAV Windows sont à la racine ; ceux de Full Tilt sont sous `SOUND/`.
Selon `midi.cpp`, Windows charge `PINBALL.MID` (avec repli MDS dans le chargeur),
Full Tilt charge `SOUND/TABA1`, `TABA2`, `TABA3`, au format MID ou MDS.
`PINBALL2.MID` est une police bitmap, pas une seconde musique.
L'exécutable historique sert de référence de comparaison ; il n'est pas
nécessaire au chargement des ressources par le fork.

Le code de sortie est 1 si des données, des noms sonores déclarés ou des musiques
sont absents, ou si une erreur est diagnostiquée, 0 sinon. Il peut donc valoir 1
sur une extraction complète (voir les absences de référence ci-dessous). Il ne
mesure ni la validité audio ni la jouabilité. Un fichier inconnu reste `unknown`,
même s'il porte le bon nom ; l'absence d'erreur ne certifie jamais son édition.
Les liens symboliques fichiers sont ignorés. Utiliser un dossier local dédié.

## État des références au 10 octobre 2026

- **Windows XP** : l'empreinte de PINBALL.EXE provient du README upstream au
  commit `cb9b7b886244a27773f66b0b19fdc2998392565e`. L'ISO fournie porte le label
  `GRTMPVOL_EN`, le marqueur SP3 et les métadonnées XP Professional anglais.
  Les 69 fichiers des sections `BaseCopyProgramFiles` et `PinballCopyFilesHelp`
  de `PINBALL.INF` ont été extraits avec `expand.exe`, dont 60 WAV et une musique
  MIDI. `PINBALL.EXE` version 5.1.2600.5512 correspond exactement au SHA-1 upstream
  `2a5b525e0f631bb6107639e2a69df15986fb0d05`.
- **Full Tilt** : le README ne précise pas le niveau de patch de son exécutable.
  Les empreintes avant/après patch et Windows 3.1 viennent du témoignage
  [upstream #214](https://github.com/k4zmu2a/SpaceCadetPinball/issues/214).
  Ce témoignage n'est pas une authentification indépendante d'un patch officiel.
  Le CD fourni (`FULL_TILT`) contient des dossiers Windows 95 et Windows 3.1.
  **La référence retenue est `WIN95/FULLTILT/CADET`** : 49 fichiers, dont 43 WAV
  et trois musiques MDS. Son `CADET.EXE` annonce 1.1 et correspond exactement au
  SHA-1 upstream `3f7b5699074b83fd713657cd94671f2156dbedc4`. Aucun patch n'a été
  appliqué. Ce binaire est distinct du binaire après patch rapporté dans #214
  (`33a12d33d19b7b891c4fccd765fde782305a9ec9`) : leur équivalence de comportement
  n'est pas établie. La variante Windows 3.1, conservée séparément (50 fichiers),
  correspond au SHA-1 `ca20c1c91acbddbd79e11cd0425728e4dc27f409` de #214.
- **Copie locale reconditionnée** : CADET.EXE compressé avec UPX, SHA-1
  `ab532f6d88c3f377d0d62aa5ce0a334a7bd12740`, puis après décompression
  `cff7a1f01a039f24260cd9e9384924ab10b19878`. Les chaînes de version annoncent
  1.1 mais les champs numériques 1.0.0.0. Aucun des deux exécutables ne correspond
  aux références documentaires. Le DAT `dd81b3488d6daa14584788bd2a045e08c820597b`
  est partagé par plusieurs éditions selon #214. Ne pas présenter cette copie
  comme un original 1.1 authentifié.

## Manifestes et limites de provenance

Les manifestes contiennent uniquement chemins relatifs, tailles, SHA-1/SHA-256,
noms audio déclarés et absences observées, sans contenu propriétaire :

- [Windows XP x86 anglais](reference-manifests/windows-xp-x86-en.json).
- [Full Tilt Windows 95 anglais, cible retenue](reference-manifests/full-tilt-win95-en.json).
- [Full Tilt Windows 3.1, variante de comparaison](reference-manifests/full-tilt-win31-en.json).

Les empreintes SHA-256 des ISO et la méthode d'extraction permettent de retrouver
les supports exacts. L'ISO Full Tilt correspond au SHA-256 annoncé par la notice
de l'archive ; celle-ci cite Win3x.org, MyAbandonware et Redump 114360. Ces mentions
ne constituent pas une vérification indépendante de provenance. Une concordance
de l'exécutable upstream consolide notre référence, sans authentifier l'ISO entière.
Les originaux, ISO, archives et rapports contenant des chemins locaux restent hors
du dépôt. Aucun installateur ni exécutable historique n'a été lancé pour cet inventaire.

## Absences audio observées sur les supports

Les deux DAT déclarent 48 noms, dont `...`, qui n'est pas un fichier sonore.
Les fichiers suivants sont également déclarés mais absents des sources extraites :

| Référence | Noms WAV absents |
| --- | --- |
| Windows XP | `sound2`, `sound37`, `sound44`, `sound52`, `sound59`, `sound62` |
| Full Tilt Windows 95 et Windows 3.1 | `sound2`, `sound37`, `sound44`, `sound59`, `sound62`, `sound111` |

Ajouter `.wav` aux noms ; les WAV Full Tilt sont sous `SOUND/`. Ces listes sont
conservées dans `absent_declared_sounds_on_reference`. Pour XP, les 69 fichiers
prévus par l'INF sont présents ; pour FT, les dossiers Cadet ont été copiés en
entier depuis le CD. Il ne s'agit donc pas d'une extraction interrompue. Toutes
les musiques attendues sont présentes. Il ne faut ni fabriquer ces fichiers, ni
les remplacer par des sons de l'autre édition, ni déclarer sans preuve qu'ils
ne sont jamais utilisés. L'observation en jeu relève de #21 et des stories audio.

## Politique d'import à reprendre dans #30

Pour un import ultérieur, préserver les fichiers sources ; présenter une variante
inconnue comme non identifiée, demander confirmation du mode et garder cette
incertitude dans sa provenance. Une empreinte DAT seule ne doit pas sélectionner
silencieusement une édition. Cette politique est documentée ici ; l'interface
d'import relève de la story #30. Les sons sont identifiables par les manifestes,
même lorsque le catalogue abrégé renvoie `unknown` pour un WAV.

Comparer les alertes à la référence de même DAT (empreinte exacte) et de même
organisation. Afficher séparément les absences déjà observées sur cette référence
et les fichiers supplémentaires perdus ou modifiés. Ne pas appliquer ces exceptions
à un DAT inconnu et ne pas utiliser le seul code de sortie de l'inventaire pour
refuser l'import. Une absence de l'exécutable historique n'empêche pas de charger
les ressources, mais interdit de prétendre l'avoir reconnu.

La story #20 est validée par comparaison des empreintes documentées, inventaire
des trois dossiers et vérification des manifestes. La validation de jouabilité,
d'audio et d'équivalence des comportements reste dans #21 et les stories de fidélité.
Aucun binaire, DAT, son ou musique propriétaire n'est inclus dans ce dépôt.
