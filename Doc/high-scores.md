# Records par mode

Les records utilisent le même fichier de préférences upstream, avec des clés
séparées : `HighScores.windows.*`, `HighScores.full-tilt.*` et
`HighScores.full-tilt-demo.*`. Le profil est fixé lors du chargement du tableau ;
le tableau précédent est sauvegardé avant le chargement d'un autre profil.
Les saisies et effacements mettent immédiatement à jour les préférences, que
le mécanisme existant d'ImGui enregistre sur disque. La sortie normale sauvegarde
aussi le tableau, comme auparavant.

Les anciennes clés `0.Name`, `0.Score` à `4.Name`, `4.Score` et `Verification`
restent intactes. Elles ne sont copiées dans aucun mode : leur origine n'est pas
connue. Si leur somme de contrôle est valide, la fenêtre des records propose
« Records hérités - mode inconnu (lecture seule) ». Cette vue ne permet ni saisie
ni effacement. Une archive invalide reste conservée dans les préférences, même
si elle n'est pas affichée. Les tableaux historiques commencent vides.

L'espace `HighScores.custom.<preset-id>.*` est réservé aux futurs remixes ; aucun
remix n'est implémenté ici. Le mode Full Tilt n'est pas présenté comme une édition
1.1 authentifiée par le seul choix de profil.

## Vérification sans ressources propriétaires

Le test ne dépend ni de SDL ni des fichiers originaux :

```sh
c++ -std=c++11 -Wall -Wextra -Werror tests/high_score_storage_test.cpp -o high_score_storage_test
./high_score_storage_test
```

Il vérifie les écritures/lectures successives de deux profils, l'archive héritée
en lecture seule, l'effacement isolé, les espaces démo/custom, les réglages
invalides et les scores élevés. Avec Zig, remplacer `c++` par `zig c++`.

## Vérification de la persistance réelle

`tests/high_score_integration_test.cpp` lie le code réel du jeu, des options et
d'ImGui. Il utilise un fichier synthétique unique, sans ressources propriétaires
ni fenêtre. Le script Windows compile avec les assertions actives et exécute
trois processus successifs :

```powershell
./tests/run_high_score_integration.ps1 -Zig <zig.exe> -Sdl <SDL2> -Mixer <SDL2_mixer> -OutputDirectory <sortie>
```

Le premier processus inscrit Windows à 1 000 000 et Full Tilt à 2 000 000,
alterne les profils puis détruit/recrée le contexte ImGui, comme lors d'un
redémarrage interne. Le deuxième relit les scores et efface seulement Windows.
Le troisième vérifie Windows vide et Full Tilt intact. Les trois vérifient
le record hérité non attribué à 3 000 000. L'enregistrement disque est déclenché
par la destruction normale du contexte, après `options::uninit()` et
`high_score::write()`, sans écriture de fichier simulée. La saisie et l'effacement
du tableau sont faits directement ; ce test ne prétend pas cliquer les dialogues.

Exécuté le 17 octobre 2026 avec Zig 0.13.0, SDL 2.30.11 et SDL_mixer 2.8.0 :
les trois étapes et le test unitaire passent. La compilation complète du jeu
avec le script `tools/build_comparison_windows.ps1` réussit également.

Contrôle visuel du 17 octobre 2026 sur ce build, avec une copie des préférences
synthétiques : le tableau Windows effacé est vide, le tableau Full Tilt affiche
« FT player » à 2 000 000, et la vue héritée affiche « Legacy player » à 3 000 000
sans bouton d'effacement. Les deux éditions ont été lancées avec leurs ressources
locales ; aucun record utilisateur n'a été utilisé. Ce contrôle vérifie l'affichage,
pas la saisie d'un score gagné en partie.

La vérification interactive complémentaire consiste à saisir un record dans
chaque mode, quitter normalement et relancer ; consulter les records hérités,
puis effacer un tableau courant et vérifier que l'autre tableau et l'archive
restent inchangés. Ce parcours nécessite un jeu de ressources jouable.
