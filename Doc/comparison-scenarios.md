# Rejouer les scénarios Windows / Full Tilt

Procédure et premier corpus de la [story #21](https://github.com/boisdal/SpaceCadetPinball/issues/21),
après l'[identification des références](reference-inventory.md).
Ce corpus teste le démarrage, le lancement et le score. Il ne démontre pas
l'équivalence physique des jeux. La course des flippers est validée par le retour
manuel de l'utilisateur ; la cadence effective reste à mesurer.

## Compiler sous Windows x64

La procédure légère vérifiée utilise **Zig 0.13.0**, **SDL 2.30.11 VC x64** et
**SDL_mixer 2.8.0 VC x64**, déjà extraits localement. Elle ne télécharge rien.
Les distributions de développement VC contiennent les en-têtes, `.lib` et DLL.
Le compilateur Zig fournit sa bibliothèque C++ ; Visual Studio n'est pas requis.
Depuis la racine du dépôt, sous PowerShell :

```powershell
./tools/build_comparison_windows.ps1 `
  -Zig C:/outils/zig-0.13.0/zig.exe `
  -Sdl C:/outils/SDL2-2.30.11 `
  -Mixer C:/outils/SDL2_mixer-2.8.0 `
  -OutputDirectory C:/pinball-local/build
```

Le script compile les `.cpp` du jeu en C++11 avec `-O2 -DNDEBUG`, lie `winmm` et
les deux bibliothèques SDL, puis copie leurs DLL à côté de l'exécutable. Un échec
de compilation interrompt le script. L'icône et les métadonnées `.rc` ne sont pas
incluses dans ce build d'audit ; le build de distribution CMake reste inchangé.
La procédure est reproductible, pas garantie identique octet pour octet : les
chemins de build peuvent modifier l'empreinte du binaire.

## Préparer et lancer les quatre configurations

Conserver les ressources hors du dépôt. Utiliser un dossier XP contenant
`PINBALL.DAT`, les WAV à la racine et les fichiers auxiliaires, et un dossier
Full Tilt contenant `CADET.DAT` et `SOUND/`. Ne pas mélanger les deux ensembles.
Comparer leurs empreintes aux manifestes de #20 avant de commencer. Les absences
audio déjà documentées ne doivent pas être corrigées avec des sons d'une autre
édition. Faire les essais dans des copies de travail des données si nécessaire.

Exemple de lancement du fork, sans recopier les ressources près du binaire :

```powershell
$forkExe = 'C:/pinball-local/build/SpaceCadetPinball.exe'
$xpData = 'C:/pinball-local/windows-xp/Pinball'
$ftData = 'C:/pinball-local/full-tilt/WIN95/CADET'
Start-Process -FilePath $forkExe -WorkingDirectory $xpData
# Quitter le jeu avant de lancer l'autre mode.
Start-Process -FilePath $forkExe -WorkingDirectory $ftData
```

Le répertoire de travail passe avant le répertoire du binaire et celui des
préférences dans `winmain.cpp`. Vérifier le journal `Loading game from:` et
l'illustration du panneau de score : logo violet Windows ou vaisseau Full Tilt.
Le titre de fenêtre du fork reste « 3D Pinball for Windows » dans les deux modes.

Pour chaque original, lancer son exécutable avec son dossier comme répertoire
de travail : `pinball.exe` pour XP, `CADET.EXE` pour Full Tilt **WIN95**. Ne pas
utiliser la variante WIN31 ni le repack UPX. Vérifier aussi le chemin du processus
réel et son empreinte : sur le poste d'essai, le lancement XP a utilisé la copie
déjà installée dans Microsoft Games, dont le SHA-1 correspond exactement à #20.
La comparaison SHA-256 des 69 fichiers du manifeste donne 67 correspondances,
dont le DAT et les ressources de jeu ; seuls `pinball.chm` et `pinball.hlp`
diffèrent ou manquent. L'aide de cette installation n'est donc pas validée.

```powershell
Start-Process -FilePath "$xpData/pinball.exe" -WorkingDirectory $xpData
Start-Process -FilePath "$ftData/CADET.EXE" -WorkingDirectory $ftData
```

Faire les mesures avec **un seul jeu au premier plan**. Une capture d'une fenêtre
inactive peut être figée ou noire ; la réactiver avant de conclure à un défaut.
Les changements de focus, menus et dialogues peuvent suspendre la simulation.

## Réglages et environnement du relevé

Session du 10 octobre 2026, Windows 11 Professionnel x64, build 26300
(`10.0.26300`), sans VM ni patch des originaux. Fork source : `3fa1649`,
version affichée dans le journal `2.1.1 DEV`, rendu SDL `direct3d`.
Les réglages ont été relevés dans les dialogues et les préférences existantes,
sans effacer les scores sauvegardés.

| Réglage | Original XP | Original Full Tilt | Fork, deux modes |
| --- | --- | --- | --- |
| Référence | 5.1.2600.5512, manifeste XP | 1.1, manifeste WIN95 | sources `3fa1649` |
| Joueurs | 1 | 1 | 1 |
| Affichage | fenêtré | fenêtré, maximum 1024 × 768 | fenêtré, résolution automatique |
| Sons / musique | activés / désactivée | activés / désactivée | activés / désactivée |
| Flipper gauche / droit | V / B, registre existant | Tab / Page suivante, dialogue F8 | Z / barre oblique, préférences |
| Lanceur | Espace | Ctrl | Espace |
| Cadence configurée | pas de réglage relevé | pas de réglage relevé | 60 FPS, 120 UPS, UPS non débridés |
| Cadence effective mesurée | non mesurée | non mesurée | non mesurée |

`options.h` indique environ 120 mises à jour/s pour l'original : c'est une
**indication statique upstream**, pas une mesure des deux exécutables sur ce PC.
La fréquence de l'écran ou des captures ne peut pas remplacer cette mesure.
L'essai du compteur de diagnostic XP (`hidden test`, puis Y) n'a pas fourni de
compteur lisible ; aucune valeur n'en est déduite. Cet essai est postérieur aux
relevés de score XP et ne les valide pas.

Full Tilt s'est initialement affiché partiellement hors écran. Déplacer la fenêtre
par sa barre de titre permet de lire la table. La tentative de changement de
résolution n'a pas établi de passage en 640 × 480 ; le réglage enregistré reste
« maximum ». Un dialogue « Unable to find other tables » a été observé lors d'un
nouveau lancement, puis acquitté ; les deux autres tables ne font pas partie de
cette extraction. Ne pas attribuer cet incident au fork.

Les journaux du fork dans les deux modes mentionnent `Couldn't open timidity.cfg`. La musique était
désactivée ; cette session ne valide donc ni la musique ni la fidélité audio.

## Corpus à rejouer

Exécuter chaque scénario sur original XP, fork Windows, original Full Tilt et
fork Full Tilt. Les intitulés de menus sont ceux de l'interface anglaise.
Utiliser les menus si un raccourci injecté n'a pas d'effet visible. Relever le
résultat après stabilisation, pas uniquement dans l'image immédiatement après
la commande. Attendre que la bille soit présente dans le couloir de lancement.

### S1 — Score acquis puis nouvelle partie

1. `Game > New Game` ; attendre le joueur 1, bille 1, score 0.
2. `Game > Launch Ball`. Ne pas secouer la table ni utiliser de triche.
3. Laisser la bille circuler jusqu'à obtenir un score positif. Photographier ou
   noter score et numéro de bille ; le score exact n'est pas une valeur attendue.
4. `Game > New Game` ; constater le retour à joueur 1, bille 1, score 0.

**Réussite :** points effectivement inscrits puis score remis à zéro. Le scénario
ne contrôle ni le barème d'une cible particulière ni la conservation des records.
Si aucun point n'est inscrit, relever ce résultat et rejouer une nouvelle partie ;
ne pas le remplacer par une réussite supposée.

### L1 — Lancement par menu

1. Nouvelle partie, attendre la bille immobile dans le lanceur.
2. `Game > Launch Ball` une seule fois.
3. Observer la sortie du couloir et la circulation sur la table. Relever le score
   ultérieur et les messages éventuels (`Re-Deploy`, mission, etc.).

**Réussite :** bille sortie du lanceur, jeu poursuivi, sans erreur bloquante.
Dans le fork, `pb::launch_ball` demande `PlungerLaunchBall`, qui applique
`MaxPullback` dans `TPlunger.cpp`. Ce choix évite de dépendre d'une durée de
maintien d'Espace injectée par l'outil. Il ne mesure pas la charge progressive.

### F1 — Course des flippers (validée manuellement)

Avant lancement, vérifier les touches dans les contrôles du jeu. Maintenir le
flipper gauche environ une seconde, relâcher, puis faire de même à droite.
Observer et enregistrer repos → levé → repos pour chaque flipper ; le score doit
rester nul. Refaire après lancement pour vérifier le contact avec une bille.
Relever séparément animation, contact et latence, sans conclure de l'un à l'autre.

Les taps automatisés de cette session n'ont pas fourni de capture montrant toute
la course. Ce premier passage automatisé ne validait donc pas F1.
L'API utilisée fournit un appui bref, sans paramètre de maintien : ne pas présenter
ces taps comme le protocole d'une seconde ci-dessus.

Après le protocole manuel fourni dans la conversation, l'utilisateur confirme :
« Les flippers marchent parfaitement dans tous les cas. Pas de retard, pas de
mouvement saccadé. » F1 est donc validé par observation humaine pour les quatre
configurations. L'absence de retard perceptible n'est pas une mesure de latence
en millisecondes ni de cadence FPS/UPS.

## Résultats du premier passage

Les scores sont des relevés à des instants différents, pas des différences de
barème ni un benchmark. La méthode est rejouable, les trajectoires ne sont pas
synchronisées et aucune graine aléatoire commune n'a été imposée.

| Cible | Score positif lu | Nouvelle partie : score 0 / bille 1 | Lancement L1 | F1 |
| --- | ---: | --- | --- | --- |
| Original XP | 3 500 puis 13 500 | observé | observé | validé par l'utilisateur |
| Fork Windows | 2 000, avec `Re-Deploy` | observé par menu | observé | validé par l'utilisateur |
| Original Full Tilt | 5 250 puis 15 250 | observé après F2 | observé | validé par l'utilisateur |
| Fork Full Tilt | 39 250, bille 2 | observé par menu | observé | validé par l'utilisateur |

Un second lancement Full Tilt original a aussi produit 6 750 points lors d'une
capture après un tap gauche ; ce relevé est séparé du passage sans commandes.

Le [relevé structuré](comparison-runs/2026-10-10.json) conserve l'environnement,
les valeurs, les limites et les empreintes des captures locales retenues. Les
captures contiennent des graphismes propriétaires : elles restent **hors Git**,
comme les exécutables, les ressources, les paramètres personnels et les journaux
bruts. Seuls la méthode, les résultats textuels et les empreintes sont versionnés.

## Niveau de preuve et suite

- **Analyse statique :** sélection du DAT, commande de lancement à puissance
  maximale, valeurs de cadence par défaut. Ne prouve pas l'exécution historique.
- **Observation interactive :** fenêtres, commandes, scores et messages ci-dessus.
  Les captures sont ponctuelles ; elles ne prouvent pas une fréquence ni une latence.
- **Automatisé :** compilation, calcul des empreintes, contrôles de structure du
  corpus. Pas de mesure automatisée de physique ou de cadence dans ce passage.

Pour terminer #21, relever la cadence
effective, avec outil, durée d'échantillonnage, pauses et méthode explicités.
Une vidéo peut vérifier l'animation ; des FPS vidéo seuls ne mesurent pas les UPS
du moteur. Le scénario de score et L1 sont réalisés pour les quatre cibles.
La matrice détaillée des écarts de règles relève de #22 ; ne pas déduire un défaut
confirmé des seuls scores finaux différents de ce corpus.

## Écart signalé pendant la validation manuelle : entry lanes Full Tilt

L'utilisateur observe que l'original Full Tilt permute les lumières des entry
lanes dans le même sens, quel que soit le flipper. Le fork Full Tilt permute dans
des sens opposés selon le flipper, comme XP. C'est un écart rapporté par comparaison
interactive de l'utilisateur, corroboré côté fork par l'analyse statique :
`control::LeftFlipperControl` envoie `TLightGroupStepBackward`, tandis que
`RightFlipperControl` envoie `TLightGroupStepForward`, sans branche `FullTiltMode`.
Les deux fonctions commandent `bmpr_inc_lights` et `ramp_bmpr_inc_lights`.

Pour rejouer : obtenir une configuration asymétrique des lumières (par exemple
une seule allumée), puis comparer une pression gauche et une pression droite
depuis le même état, sans passage de bille entre les deux. Noter l'ordre spatial
des lumières avant/après sur l'original et le fork. Une configuration uniforme
ne permet pas de voir le sens de rotation.

Le sens unique exact de l'original et le comportement de l'autre groupe de
lumières ne sont pas établis par ce retour. Ne pas déduire de la présence des
deux groupes dans le code que les deux sont concernés. Écart à reprendre dans
la matrice #22, puis dans une correction de fidélité ; aucune correction de
règles n'est incluse dans le corpus #21.
