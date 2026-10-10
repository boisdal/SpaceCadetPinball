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

La vérification interactive complémentaire consiste à saisir un record dans
chaque mode, quitter normalement et relancer ; consulter les records hérités,
puis effacer un tableau courant et vérifier que l'autre tableau et l'archive
restent inchangés. Ce parcours nécessite un jeu de ressources jouable.
