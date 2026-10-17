# Sons des cibles de mission — #24

La collision avec une cible de mission joue `soundwave49D` en Full Tilt,
ou `soundwave52` quand les trois lampes du groupe sont allumées après le coup.
Le mode Windows conserve sa condition existante : `soundwave52` si `lite198`
est éteinte ou clignote, `soundwave49D` sinon. Les points, les bits de cibles
touchées et l'extinction différée du groupe restent inchangés.

## Preuve historique

Vérification directe le 17 octobre 2026 sur `WIN95/CADET/CADET.EXE`, SHA-1
`3f7b5699074b83fd713657cd94671f2156dbedc4` (référence #20).
À `0x425272–0x4252ba`, le contrôleur interroge le groupe avec le message `0x29`,
compare son résultat à 3, joue le premier son si égal, l'autre sinon. Les noms
proviennent des pointeurs de chaînes des tags du binaire, pas d'une déduction
sur le numéro du fichier WAV :

| Objet | Emplacement du pointeur de composant | Nom référencé par le tag |
| --- | --- | --- |
| Groupe interrogé | `0x4384d4` | `ramp_tgt_lights` |
| Trois lampes | `0x438a4c` | `soundwave52` |
| Autres cas | `0x438af4` | `soundwave49D` |

Les pointeurs de noms se trouvent quatre octets avant chacun de ces emplacements.
Cette preuve confirme le choix de l'événement sonore Full Tilt. Elle ne prouve
pas à elle seule la fidélité acoustique du mélangeur moderne.

## Vérifications exécutées

```powershell
python tools/test_mission_target_sound.py --cxx <chemin-vers-zig.exe> --zig
```

Le test compile les corps réels de `MissionSpotTargetControl`,
`Sound::LoadWaveFile`, `Sound::PlaySound` et `loader::play_sound` avec des doubles
observables. Il ne maintient pas une seconde implémentation de la sélection.

- 72 cas : deux modes × une/deux/trois lampes × mission allumée/éteinte ×
  clignotement actif/inactif × les trois cibles. Vérification du son, du caller,
  des points, du bit de cible et du signal d'extinction du groupe.
- Un événement sans collision ne produit ni score ni son.
- Un WAV inexistant est réellement ouvert, renvoie `nullptr`, puis traverse
  le loader et la garde du mélangeur sans aucun appel SDL_mixer. Les identifiants
  absents/invalide (0/-1) sont également silencieux.
- Le même test échoue sur `origin/master` à `0a91c74` sur la sélection du son FT,
  puis réussit avec la correction.
- Compilation Windows x64 C++11 optimisée réussie avec Zig 0.13.0,
  SDL 2.30.11 et SDL_mixer 2.8.0 via `build_comparison_windows.ps1`.

Les doubles fixent le nombre de lampes après le coup et remplacent SDL_mixer :
ils ne constituent ni une partie interactive ni une écoute comparée. Cette
dernière reste à réaliser. Les diagnostics nominaux de fichiers audio absents
dans l'interface d'import sont traités avec #30 ; cette correction ne change
pas l'import et ne prétend pas avoir validé son interface.
