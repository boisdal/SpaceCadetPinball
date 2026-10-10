# Relevé de cadence du 11 octobre 2026

Complément du [corpus comparatif #21](comparison-scenarios.md). Le scénario C1
utilise `Game > Demo` sur les originaux. Pour le fork, utiliser `Game > New Game`,
puis `Game > Launch Ball`, sans autre entrée pendant le relevé. Les éventuelles
attentes entre billes font partie de la fenêtre mesurée. Un premier passage
Windows en démo est conservé séparément : il est resté à « Awaiting Deployment »
et ne prouve pas une activité physique de la bille. Il a donc été complété par
une partie lancée explicitement. Ce comportement de démo n'est pas corrigé ici.
Ce n'est ni un replay déterministe, ni une mesure de latence des flippers.
Une seule application de jeu tourne à la fois, fenêtrée au premier plan, sans
entrée utilisateur pendant la capture. Pas de compilation pendant les mesures.
Même Windows 11 x64 build 26300 et mêmes ressources identifiées dans les manifestes.

## Ce qui est mesuré

**Fork :** un build d'audit optionnel horodate, avec `std::chrono::steady_clock`,
chaque retour de `pb::frame(dt)` et chaque retour de `SDL_RenderPresent`.
Il compte donc séparément les mises à jour de la boucle et les présentations
SDL ; il ne compte ni les sous-étapes de collision ni les images effectivement
affichées par le moniteur. Les réglages restent 120 UPS / 60 FPS, UPS plafonnés,
`HybridSleep=0`, rendu `direct3d`, sons activés, musique désactivée.

Les événements restent en mémoire pendant 45 secondes. Le CSV est écrit après
la fenêtre de mesure, pour éviter des écritures disque à chaque image. La sonde
enregistre aussi les changements d'état focus/pause. Elle ne modifie ni le pas
de temps ni le limiteur. Son surcoût n'est pas mesuré séparément : les résultats
concernent le build instrumenté. Sans `PINBALL_CADENCE_AUDIT`, les points de mesure
ne sont pas compilés ; CMake et les builds normaux restent inchangés.

**Originaux :** leur diagnostic natif intitulé `Frames/sec` est lu dans le titre
de fenêtre environ toutes les 100 ms, sans injection de code ni modification des
exécutables. Le diagnostic calcule `300 / durée écoulée` après 300 passages
acceptés dans la boucle de jeu. Il utilise `timeGetTime`, en millisecondes.
Les polls répétés d'une même valeur ne sont pas des mesures indépendantes ; la
moyenne publiée est une moyenne des valeurs échantillonnées du titre, pas un
décompte exact des événements sur 45 secondes. Min/max concernent ces valeurs,
pas les intervalles entre deux images. Le relevé dure 55 secondes : les dix
premières sont écartées pour laisser sortir les effets des menus du compteur.
Le collecteur vérifie le premier plan à chaque poll ; cela ne détecte pas une
interruption plus brève que l'intervalle de polling.

**Limite Full Tilt :** la boucle peut appeler sa fonction de mise à jour avec
un delta nul, puis compter ce passage. Le diagnostic très élevé mesure donc
aussi des tours sans progression du temps simulé. Ne pas le publier comme des
FPS visibles ou comme autant de pas physiques utiles par seconde. La cadence
des seuls pas positifs et celle des présentations GDI restent non mesurées.
Cette distinction empêche une comparaison numérique directe avec les UPS/FPS
du fork. Leur analyse détaillée relève de [#25](https://github.com/boisdal/SpaceCadetPinball/issues/25).

## Reproduire la collecte

### Originaux

1. Lancer un original, rester avant le premier lancement de bille.
2. Presser réellement les touches `hidden test`, puis `Y` pour XP. Pour Full
   Tilt WIN95 1.1, utiliser `hidden`, **Tab**, `test`, puis `Y` : Espace n'est
   pas accepté comme séparateur par cet exécutable. L'injection de texte Unicode
   n'est pas équivalente aux événements clavier attendus.
3. Vérifier que le titre devient `Frames/sec = ...`, puis `Game > Demo`.
4. Laisser la fenêtre au premier plan, aucun menu ouvert, et lancer le collecteur
   depuis un processus en arrière-plan. Un terminal amené au premier plan
   invalide la mesure. Ne pas cliquer dans la table en mode diagnostic.

```powershell
# Depuis un lanceur en arrière-plan ; Python standard, aucun paquet tiers.
$jeu = Get-Process pinball  # CADET pour Full Tilt
python ./tools/measure_original_cadence.py `
  --window $jeu.MainWindowHandle --seconds 55 --output C:/pinball-local/xp.json
python ./tools/summarize_cadence.py C:/pinball-local/xp.json
```

Fermer l'original après le relevé réinitialise le diagnostic à la prochaine
exécution. Ne pas utiliser ces parties de diagnostic pour valider les records.

### Fork

Ajouter `-CadenceAudit` à la [commande de compilation](comparison-scenarios.md).
Utiliser un nom de sortie neuf par exécution : un ancien fichier `.start`
déclencherait immédiatement la mesure.

```powershell
$env:PINBALL_CADENCE_CSV = 'C:/pinball-local/fork-windows.csv'
Start-Process -FilePath C:/pinball-local/build/SpaceCadetPinball.exe `
  -WorkingDirectory C:/pinball-local/windows-xp/Pinball
# Dans le jeu : New Game puis Launch Ball ; fermer les menus, garder le focus.
# Créer ensuite ce signal depuis un processus en arrière-plan :
New-Item -ItemType File -Path "$env:PINBALL_CADENCE_CSV.start"
# Attendre l'apparition du CSV après 45 secondes, puis seulement quitter le jeu.
python ./tools/summarize_cadence.py C:/pinball-local/fork-windows.csv
```

Refaire avec les données Full Tilt et un nouveau nom de CSV. La sonde surveille
le signal au plus une fois par seconde avant la capture, puis ne touche plus
au disque avant la fin. `U` contient le dt transmis à la simulation, `R` un
retour de présentation, `S` l'état actif/non pausé et `E` la fin de capture.
Le plafond de 40 000 événements invalide une capture débridée trop volumineuse.
Le résumé refuse une capture incomplète ou avec état inactif/pausé.

## Interprétation du diagnostic natif vérifiée localement

L'analyse statique porte sur les exécutables exacts des manifestes, pas sur le
repack décompressé utilisé dans d'autres recherches. Les adresses suivantes
sont des adresses virtuelles avec leur base préférée ; aucun octet propriétaire
ni désassemblage complet n'est versionné.

| Élément | XP 5.1.2600.5512, base 0x01000000 | Full Tilt 1.1 WIN95, base 0x00400000 |
| --- | --- | --- |
| Activation du diagnostic Y | 0x01007e03 | 0x004212a9 |
| Formule du titre | 0x01008854–0x0100889c | 0x00422253–0x00422294 |
| Constante 300 | double à 0x01001630 | double à 0x004358d8 |
| Horloge importée timeGetTime | 0x01001260 | 0x0043c6c8 |
| Appel de mise à jour | 0x01008a2d | 0x00422499 |
| Décrément du compteur de 300 | 0x01008a78 | 0x004224e7 |

XP attend avec `Sleep(8)` quand le timestamp n'a pas changé
(0x01008a13–0x01008a25). Full Tilt soustrait les timestamps et appelle la mise
à jour sans ce test (0x0042248d–0x00422499). La fonction à 0x004119b0 borne
le dt à 100 ms mais n'exclut pas zéro. Ces constats expliquent pourquoi le
libellé identique `Frames/sec` n'assure pas une mesure équivalente.

## Preuves

| Cible / scénario | Cadence relevée sur 45 s | Portée |
| --- | --- | --- |
| Original XP, démo | moyenne échantillonnée 79,90 ; min 77,74 ; max 82,87 /s | diagnostic de boucle natif |
| Original Full Tilt, démo | moyenne échantillonnée 37 051,93 ; min 23 076,92 ; max 50 000 /s | tours de boucle, **y compris dt nul** ; pas des FPS visibles |
| Fork Windows, bille lancée | 5 370 mises à jour, 2 685 présentations : **119,33 UPS / 59,67 FPS** | événements instrumentés |
| Fork Full Tilt, bille lancée | 5 400 mises à jour, 2 700 présentations : **120 UPS / 60 FPS** | événements instrumentés |
| Fork Windows, démo restée en attente | 5 400 mises à jour, 2 700 présentations : 120 UPS / 60 FPS | passage complémentaire, sans preuve de bille active |

Les deux relevés actifs du fork ne contiennent aucun changement d'état vers
inactif/pausé. Pour Windows, les intervalles entre présentations ont un p99 de
17,58 ms et un maximum de **118,26 ms** ; pour Full Tilt, p99 17,64 ms et maximum
18,19 ms. La moyenne ne doit donc pas masquer l'intervalle isolé du passage
Windows. Sa cause n'est pas établie. Les dt cumulés transmis au jeu valent
44,753 s pour Windows et 45,001 s pour Full Tilt ; le limiteur existant borne
chaque dt à deux fois le pas cible. Ce constat constitue une entrée pour #25,
pas une correction ni une démonstration de stabilité sur toutes les charges.

Les mesures des originaux comportent chacune 449 polls retenus, sans valeur
manquante ni poll hors premier plan. Le titre a changé de valeur 12 fois pour
XP et 338 fois pour Full Tilt ; ce nombre n'est pas celui de tous les lots
internes, plusieurs lots pouvant produire le même texte ou survenir entre polls.

Les trajectoires, périodes d'attente et tailles de fenêtre diffèrent : ces
résultats documentent le poste et le scénario, sans classer les performances
des quatre configurations. L'affichage effectivement vu au moniteur n'est
mesuré pour aucune cible.

Les résultats et empreintes sont conservés dans
[le corpus de cadence](comparison-runs/2026-10-11-cadence.json).
Les données de mesure textuelles sont versionnées pour permettre de recalculer
les résumés (fichiers gzip ; `summarize_cadence.py` accepte aussi `.gz`). Les
captures de jeu, ressources, binaires et journaux contenant des
chemins locaux restent dans le dossier d'audit hors Git.
