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
fichiers WAV attendus viennent des groupes de type 202, pas d'une liste supposée.

Les WAV Windows sont à la racine ; ceux de Full Tilt sont sous `SOUND/`.
Selon `midi.cpp`, Windows charge `PINBALL.MID` (avec repli MDS dans le chargeur),
Full Tilt charge `SOUND/TABA1`, `TABA2`, `TABA3`, au format MID ou MDS.
`PINBALL2.MID` est une police bitmap, pas une seconde musique.
L'exécutable historique sert de référence de comparaison ; il n'est pas
nécessaire au chargement des ressources par le fork.

Le code de sortie est 1 si des données/sons/musiques sont absents ou si une
erreur est diagnostiquée, 0 sinon. Cela mesure la présence des dépendances,
pas leur validité audio ni la jouabilité. Un fichier inconnu reste `unknown`,
même s'il porte le bon nom ; l'absence d'erreur ne certifie jamais son édition.
Les liens symboliques fichiers sont ignorés. Utiliser un dossier local dédié.

## État des références au 10 octobre 2026

- **Windows XP** : l'empreinte de PINBALL.EXE provient du README upstream au
  commit `cb9b7b886244a27773f66b0b19fdc2998392565e`. Aucun jeu de fichiers XP
  n'a été fourni dans cet espace de travail ; DAT, sons et musique restent à
  relever sur la copie de référence de l'utilisateur.
- **Full Tilt** : le README ne précise pas le niveau de patch de son exécutable.
  Les empreintes avant/après patch et Windows 3.1 viennent du témoignage
  [upstream #214](https://github.com/k4zmu2a/SpaceCadetPinball/issues/214).
  Ce témoignage n'est pas une authentification indépendante d'un patch officiel.
- **Copie locale reconditionnée** : CADET.EXE compressé avec UPX, SHA-1
  `ab532f6d88c3f377d0d62aa5ce0a334a7bd12740`, puis après décompression
  `cff7a1f01a039f24260cd9e9384924ab10b19878`. Les chaînes de version annoncent
  1.1 mais les champs numériques 1.0.0.0. Aucun des deux exécutables ne correspond
  aux références documentaires. Le DAT `dd81b3488d6daa14584788bd2a045e08c820597b`
  est partagé par plusieurs éditions selon #214. Ne pas présenter cette copie
  comme un original 1.1 authentifié.

Pour un import ultérieur, préserver les fichiers sources ; présenter une variante
inconnue comme non identifiée, demander confirmation du mode et garder cette
incertitude dans sa provenance. Une empreinte DAT seule ne doit pas sélectionner
silencieusement une édition. Cette politique est documentée ici ; l'interface
d'import relève de la story #30.

La validation finale demande encore une copie XP et une provenance FT consolidée
(support d'origine et/ou patch identifié, empreintes avant et après application).
Aucun binaire, DAT, son ou musique propriétaire n'est inclus dans ce dépôt.
