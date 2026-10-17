# Physique Windows : preuves et mesures de lancement (#26)

## Résultat établi

Le menu **Jeu → Lancer la bille** doit maintenir l'impulsion maximale pendant
200 ms dans le profil Windows. Le portage des composants FT (`c1c7487`) avait
remplacé cette commande par un relâchement manuel, limité à 25 ms. Le correctif
rétablit uniquement la branche Windows de `PlungerLaunchBall` ; la commande FT
et le relâchement manuel conservent leur comportement.

Ce résultat ne démontre pas encore la fidélité générale des trajectoires.
La story #26 reste ouverte pour les flippers, rebonds, captures et éjections,
et pour l'appréciation des sensations par l'utilisateur.

## Référence authentifiée et chemin de preuve

Le fichier XP retenu est celui du manifeste `windows-xp-x86-en`, SHA256
`2bbc8234685fe2f6324040af6ea20123cf00c4a56882ce0d9074f0beefac67bc`.
L'extracteur refuse tout autre hash avant de lire des adresses fixes.

Le constructeur à `0x101a4f3` est identifié par l'attribut de position 601,
le maximum 100, le calcul entier `100 / (nombre de bitmaps * 8)`, et le délai
25 ms. Il installe la vtable `0x1002750`, dont la première entrée est
`Message` à `0x10185ad`. Le décodage des deux tables du switch donne, pour
le message 1017, la branche `0x1018691` : seuil nul, force maximale, chargement
du float 0,2 à `0x10023a4`, puis programmation de `0x10173c8`.
Ce callback réinitialise la force. Le relâchement manuel charge au contraire
le délai de l'objet, 0,025. Ce chemin recoupe l'implémentation antérieure au
portage FT, sans traiter cet ancien code comme un oracle.

Reproduction (Python standard, aucun binaire distribué) :

```powershell
python tools/inspect_windows_physics.py <référence-XP>/pinball.exe --output evidence.json
```

Le [résultat statique enregistré](comparison-runs/2026-10-17-xp-plunger-evidence.json)
contient les adresses, constantes et limites de cette preuve.

## Comparaison de l'historique

| Élément | État actuel / historique | Conclusion permise |
|---|---|---|
| Force maximale Windows 100, FT 50 | Différenciées dans le code courant | Pas de correction de force nécessaire |
| Incrément Windows entier | Rétabli par `75d2d98` après le portage FT | Ne pas réintroduire cette ancienne correction |
| Tirage Windows hors bille active | Rétabli par `18c80a0` | Déjà différencié de FT |
| Commande lancement 1017 | Modifiée par `c1c7487` de 200 à 25 ms | Écart confirmé contre le binaire XP ; corrigé ici |
| Collisions des flippers | `ba470e8` remplace l'itération temporelle par le solveur FT pour les deux profils | Différence d'architecture, pas une preuve suffisante de trajectoire incorrecte |
| Vitesse angulaire Windows | Le temps de parcours DAT est converti en radians/s | Conversion présente ; pas une preuve des angles de sortie |
| Trou de rampe | Capture Windows 0,5 s, FT immédiate, déjà différenciée | À mesurer dynamiquement avant modification |
| Kickout/sink | Changements d'ordonnancement et gestion de bille lors du portage FT | À vérifier par captures et éjections comparatives |

## Mesures reproductibles du fork

Compiler avec `tools/build_comparison_windows.ps1 -CadenceAudit` (dépendances
et paramètres décrits dans [la procédure de cadence](simulation-cadence.md)).

```powershell
python tools/run_physics_launches.py --exe <audit>/SpaceCadetPinball.exe --data <dossier-XP> --mode windows --output <nouveau-dossier>
python tools/run_physics_launches.py --exe <audit>/SpaceCadetPinball.exe --data <dossier-FT> --mode full-tilt --output <autre-nouveau-dossier>
```

Chaque scénario démarre avec des préférences isolées, une graine aléatoire fixe,
120 pas/s et 60 rendus/s, audio désactivé. Le tir commence après environ 4 s
pour laisser la bille se poser. `weak` retient le ressort 100 ms, `maximum`
3 s, `automatic` utilise la vraie commande du menu. La décision d'entrée et
la fin de timer se produisent aux frontières des pas ; les flottants et les
ticks entiers peuvent décaler une entrée d'un pas. Le fichier `physics.csv`
enregistre direction, force et seuil après chaque pas, avec la trajectoire
et les événements de cadence dans les fichiers voisins.

Une fenêtre de force de 25 ms n'implique donc pas exactement trois échantillons
strictement positifs : le premier état après commande et l'état après expiration
sont échantillonnés après le traitement du pas. La couverture des échantillons
positifs n'est pas une mesure indépendante de la durée du timer d'origine.

Ces tests n'utilisent pas les positions arbitraires ou les collisions simulées
d'un faux moteur. Ils suivent les composants réels, mais ne mesurent que le
fork. Les trajectoires de XP ne sont pas encore capturées avec les mêmes entrées.

### Résultats du 17 octobre 2026

Les 12 replays avant/après contiennent chacun 2160 états (18 s simulées).

| Profil / commande | Force mesurée | Échantillons positifs avant → après |
|---|---:|---:|
| Windows, faible | 5 | 2 → 2 |
| Windows, maximal manuel | 100 | 2 → 2 |
| Windows, menu automatique | 100 | 2 → 23 |
| FT, faible | 3,90625 | 2 → 2 |
| FT, maximal manuel | 50 | 2 → 2 |
| FT, menu automatique | 50 | 2 → 2 |

La fenêtre du menu Windows gagne 21 pas, soit 175 ms. Les trajectoires des six
scénarios restent identiques avant/après, y compris pour ce menu : dans cette
situation, la bille déjà posée touche le lanceur pendant les 25 premières ms.
Le correctif restaure la durée de disponibilité de l'impulsion ; ces mesures
ne démontrent donc aucun changement de trajectoire ni de sensation en jeu normal.
Les états de force/seuil/direction sont également identiques dans les cinq
scénarios qui ne doivent pas changer, dont les trois FT.

[Synthèse et empreintes](comparison-runs/2026-10-17-issue26-launches.json),
[traces numériques compressées](comparison-runs/physics-issue26-2026-10-17/).
Vérification et archivage à partir des quatre dossiers de mesures :

```powershell
python tools/compare_physics_launches.py <racine-mesures> --traces <dossier-traces> --output launches.json
```

La racine contient `windows-baseline`, `windows-fixed`, `full-tilt-baseline`
et `full-tilt-fixed`. La baseline utilise le même audit et le même code que
le correctif, sauf `TPlunger.cpp` remis à `76fb708`. Les exécutables d'audit
sont compilés avec Zig 0.13.0, SDL 2.30.11 et SDL_mixer 2.8.0.

## Suite de la comparaison

La comparaison historique doit contrôler la cadence et le générateur aléatoire,
séparer les tirs manuels de la commande automatique et aligner les événements
de collision. Sur la référence et le fork : relever lancement faible/maximal,
tenue de bille sur chaque flipper, angles après frappe à différentes positions,
rebonds sur les parois, entrée/sortie de rampe, trou et kickout. Conserver les
captures et événements avec leur provenance et leurs incertitudes. Une divergence
de score seule ne localise pas un défaut physique. Corriger ensuite seulement
les différences confirmées, en rejouant aussi FT. L'utilisateur appréciera enfin
les sensations ; son retour antérieur sur la fluidité ne valide pas à lui seul
les trajectoires historiques.
