# Cadence indépendante de l'affichage — #25

## Diagnostic et choix

Le [relevé #21](cadence-measurements.md) contient trois interruptions entre
mises à jour de 118,1904, 102,9547 et 109,5377 ms. Elles se terminent vers
9,2517, 9,5637 et 11,2738 secondes. Le plus grand intervalle de présentation
est de 118,2603 ms. La capture ne permet pas de distinguer traitement des
événements, physique, audio ou désordonnancement du processus pendant ces
intervalles. Il serait incorrect d'en attribuer rétrospectivement la cause au
pilote graphique ou à l'audio.

L'ancienne boucle utilisait le temps du tour précédent comme dt physique,
plafonné à deux pas cibles. À 120 UPS, un arrêt de 118 ms ne pouvait donc
transmettre que 16,67 ms au jeu. Elle liait également le rendu au nombre de
tours et augmentait les UPS lorsqu'on augmentait les FPS au-delà de leur valeur.
Ainsi, changer l'affichage pouvait changer les pas physiques et la trajectoire.

La référence choisie pour les deux profils du fork est **120 pas fixes par
seconde**, soit 8,333333 ms : c'est le réglage par défaut du fork. Ce choix ne
prétend pas reproduire le temps CPU ni le compteur natif des originaux. Les UPS
restent réglables, mais les FPS ne les modifient plus. Une comparaison physique
doit conserver les mêmes UPS, données de jeu, entrées et graine aléatoire.

## Comportement livré

- Le temps réel s'accumule ; la simulation consomme uniquement des pas entiers.
  Les fractions et le retard court sont conservés.
- Huit pas au maximum sont exécutés avant de repasser par la lecture des
  événements. Un arrêt de 118 ms peut être rattrapé en deux passages.
- Le rendu possède sa propre échéance. Les présentations manquées sont sautées,
  sans envoyer une rafale d'images périmées. Au-delà des UPS, certaines images
  montrent le même état : aucune interpolation n'est ajoutée.
- Pause, perte de focus et reprise réinitialisent la dette de simulation.
  Une interruption exceptionnelle conserve au maximum 250 ms de dette ; le
  surplus est abandonné pour éviter un rattrapage interminable. La sonde le
  signale avec `L` (millisecondes abandonnées).
- L'ancien réglage de benchmark « Uncapped UPS » devient « Disable sleep
  (benchmark) ». Il désactive l'attente, sans modifier le pas physique ni les
  échéances de présentation. La clé de configuration existante reste compatible.

Les entrées normales sont lues avant les pas physiques ; elles prennent effet
sur le prochain pas disponible. La quantification nominale est d'un pas
(8,33 ms à 120 UPS), à laquelle s'ajoutent les délais du système et du rendu.
Une boucle sur un seul thread ne peut pas présenter des images ni lire le
clavier pendant qu'un appel système bloque. La correction conserve le temps
simulé après un blocage court ; elle ne garantit pas un maximum absolu de
latence visuelle sous toute charge.

## Protocole reproductible

Compiler avec `tools/build_comparison_windows.ps1 -CadenceAudit` et les mêmes
dépendances que #21 (Zig 0.13.0, SDL 2.30.11, SDL_mixer 2.8.0). Le build normal
ne contient ni replay, ni sonde, ni variables d'environnement d'audit.

```powershell
python tools/run_cadence_matrix.py `
  --exe C:/pinball-local/build/SpaceCadetPinball.exe `
  --data C:/pinball-local/windows-xp/Pinball `
  --output C:/pinball-local/measurements/windows `
  --case xp60:60:800:600:0 `
  --case xp90:90:1280:900:0 `
  --case xp144:144:640:480:0 `
  --case xp-stall:60:1280:900:118 `
  --case xp-resize:60:800:600:0:resize `
  --case xp-hidpi:90:800:600:0:hidpi
```

Refaire avec les ressources Full Tilt WIN95/CADET et un autre dossier de sortie.
Chaque processus utilise un fichier INI propre : les réglages et records de
l'utilisateur ne sont pas modifiés. Le programme quitte automatiquement après
18 secondes simulées. Garder le jeu au premier plan ; ne pas compiler ni lancer
un autre jeu pendant la mesure. Le collecteur rejette les captures incomplètes,
les pertes de focus, les séquences incomplètes, les flippers immobiles et les
parties sans preuve de lancement et de score.

Le replay saute l'animation d'introduction avec un second New Game, fixe la
graine à 1, laisse 500 ms réels à la fenêtre pour se stabiliser sans avancer la
simulation, puis lance la bille à 4 secondes simulées. Les entrées flippers
alternées et simultanées passent par `pb::InputDown/InputUp` toutes les 500 ms
de 4,5 à 16 secondes. Il s'agit d'entrées au temps de simulation, pas d'une
mesure de latence clavier matérielle. Le scénario `stall` attend 118 ms dans
la phase de présentation vers 9 secondes simulées. Le scénario `resize` passe
à 1100×740, 144 FPS et une échelle d'interface de 2 à ce même instant.
`hidpi` active `SDL_WINDOWS_DPI_SCALING=1` avant le démarrage de SDL ; le journal
conserve taille logique, taille de sortie et DPI réellement rapporté.

Les sondes accumulent les événements en mémoire puis écrivent après la mesure :
`U` = pas simulé (dt ms), `R` = retour de présentation, `S` = focus/pause,
`I` = traitement des événements, `P` = mise à jour avec instrumentation,
`A` = appel audio, `D` = préparation et présentation de l'image,
`V` = présentation, `W` = attente, `J` = interruption injectée,
`K` = numéro d'entrée, `X` = changement d'affichage, `L` = temps abandonné,
`E` = fin. Les durées des phases se recouvrent (`A` dans `P`, `V` dans `D`) :
ne pas les additionner. La capacité est de 180 000 événements ; son dépassement
invalide la capture.

Le CSV de trajectoire conserve chaque pas : temps simulé, position/vitesse de
la première bille, angles des deux flippers, score, activité et nombre de billes.
Comparer uniquement les traces du même profil, même scénario et même build.

```powershell
python tools/summarize_cadence_replay.py xp90.csv xp90.csv.trajectory.csv `
  --compare xp60.csv.trajectory.csv
```

Les premiers essais de mise au point (lancement avant la fin de l'introduction
ou avant stabilisation de la bille) ont été rejetés. Ils ne servent pas de preuve
de jeu actif. Le résumé historique de 45 secondes de #21 ne doit pas être utilisé
pour les captures de replay, qui terminent sur un temps simulé.

## Vérification du planificateur

`tools/test_frame_scheduler.cpp` vérifie les comptes de pas à 60/90/120/144/360
FPS, la conservation d'un retard de 118 ms et de sa fraction, la limite par
passage, l'absence de rafale de présentations, pause/reprise, le plafond de dette
et le changement explicite d'UPS. Compiler sans `NDEBUG` pour garder les assertions.

Les mesures de rendu sont des retours de `SDL_RenderPresent`, pas les images
réellement vues par le moniteur. La tolérance temporelle inclut un pas à chaque
bord de capture et la gigue de l'ordonnanceur. La comparaison des trajectoires
vise l'égalité des valeurs enregistrées sur ce build ; elle ne promet pas une
reproductibilité bit à bit entre compilateurs ou plateformes.

## Résultats du 11 octobre 2026

Les [résumés et empreintes](comparison-runs/2026-10-11-issue25.json) référencent
30 fichiers numériques gzip : cadence et trajectoire pour chacun des 15 passages
validés. Les binaires et ressources propriétaires ne sont pas versionnés.

| Profil corrigé | FPS / fenêtre | Intervalle de présentation p99 / maximum (ms) | Score final |
| --- | --- | --- | --- |
| Windows | 60 / 800×600 | 17,555 / 18,128 | 20 750 |
| Windows | 90 / 1280×900 | 12,222 / 12,725 | 20 750 |
| Windows | 144 / 640×480 | 8,177 / 8,825 | 20 750 |
| Full Tilt | 60 / 800×600 | 17,692 / 19,976 | 10 000 |
| Full Tilt | 90 / 1280×900 | 12,163 / 12,656 | 10 000 |
| Full Tilt | 144 / 640×480 | 8,211 / 8,773 | 10 000 |

Chaque profil conserve **exactement les mêmes 2 160 échantillons de trajectoire**
dans ses six variantes : les trois lignes ci-dessus, interruption injectée,
redimensionnement avec changement de FPS/échelle, et DPI. Cette égalité porte
sur toutes les colonnes enregistrées, pas seulement le score. Les 12 passages
corrigés conservent un dt unique de 8,333333015 ms (représentation `float`) et
ne signalent aucun abandon de temps (`L`). Les écarts aux 18 secondes réelles
correspondent aux bords de capture et à l'attente finale, inférieurs à 10 ms.

Le mode DPI actif rapporte **144 DPI**, une fenêtre logique de **800×600** et
une sortie de **1200×900** ; le mode non conscient du DPI rapporte 96 DPI et
800×600. Il s'agit donc bien d'un changement de mise à l'échelle effective,
sans modification du réglage global de Windows. Le déplacement entre plusieurs
moniteurs physiques n'a pas été testé.

Les changements d'affichage en cours de partie produisent un intervalle maximal
de 26,559 ms (Windows) et 24,902 ms (Full Tilt), sans modifier les traces physiques.
L'arrêt injecté produit volontairement des intervalles visibles de 135,378 et
135,668 ms : ces images manquées ne sont pas dissimulées par la moyenne.

Pour mesurer la perte permanente, comparer `temps réel − somme des dt` au premier
pas suivant 8 puis 12 secondes simulées, de part et d'autre de l'arrêt injecté :

| Boucle / profil | Variation du retard réel sur simulé |
| --- | --- |
| Ancienne boucle Windows, arrêt injecté | **+101,976 ms** |
| Boucle corrigée Windows, arrêt injecté | **+0,314 ms** |
| Boucle corrigée Full Tilt, arrêt injecté | **−0,326 ms** |

Les deux derniers écarts sont inférieurs à un pas et relèvent de la gigue du
point d'échantillonnage. Les trajectoires avec interruption sont identiques à
celles sans interruption, ce qui vérifie aussi la conservation de tous les pas.
Avant correction, les passages Windows à 60 et 120 FPS donnent respectivement
4 500 et 21 250 points avec des trajectoires différentes malgré la même graine
et les mêmes commandes au temps simulé. C'est une démonstration de sensibilité
aux dt variables, pas une cible de score ni une comparaison de fidélité historique.

Aucune interruption spontanée comparable à celles de #21 n'a été reproduite
dans ces 15 passages actifs. **La cause initiale du pic reste inconnue.** Les
sondes par phase restent disponibles pour la localiser si elle réapparaît.
Le défaut démontré et corrigé ici est la dépendance des pas physiques à
l'affichage et la perte de temps après une interruption courte.

### Provenance et contrôles

Le build corrigé mesuré provient de `4d89b17`. Le build de comparaison reprend
ce même commit, avec `winmain.cpp`, `winmain.h` et `options.cpp` de `5046de4`
pour restaurer la boucle antérieure tout en conservant le replay corrigé.
Les SHA-256 des trois binaires compilés (comparaison, corrigé instrumenté,
normal) figurent dans le JSON. Le binaire normal a été compilé depuis `ef4d937` ;
les changements suivants du scénario d'audit n'entrent pas dans ce build.

Vérifications effectuées : compilation C++11 optimisée avec et sans sonde,
assertions de `test_frame_scheduler.cpp`, quatre tests d'inventaire de références,
validation des 15 captures, égalité des six trajectoires de chaque profil,
recalcul des résumés depuis les archives et absence des variables d'audit dans
le binaire normal. Les sensations validées manuellement pour #21 constituent
le retour utilisateur antérieur ; aucun nouveau retour manuel n'est inventé
pour ce changement de pas physique. Les règles Full Tilt de #33 sont inchangées.
