# Collisions et multibille Full Tilt — #27

Relevé du 17 octobre 2026, source `3d5b0b0`, puis extension d'intégration décrite
ci-dessous. Aucun changement de physique
n'est justifié par les vérifications ci-dessous. La story reste ouverte : les
tests de composants ne remplacent pas la comparaison des trajectoires avec
CADET.EXE, ni l'appréciation des sensations par l'utilisateur.

## Locks et lancement : preuve historique

La référence WIN95 est imposée par SHA-1
`3f7b5699074b83fd713657cd94671f2156dbedc4`. L'extracteur refuse toute autre
empreinte avant de lire les adresses. Les contrôles enregistrés dans
[le relevé statique](comparison-runs/2026-10-17-ft-multiball-evidence.json)
portent sur les instructions du binaire original, pas sur l'ancien désassemblage.

À `0x422f30`, le lock n'agit que si le compteur actif vaut au plus 1. Il retire
la bille capturée, puis compare les locks précédents à 2. Les deux premiers
locks augmentent ce compteur et demandent une bille de remplacement au lanceur
avec le message 1019 et un délai de 2 s. Le troisième appelle `0x422d20`, qui
ajoute trois au compteur actif et programme les trois sinks. Le fork reproduit
ces décisions dans `table_bump_ball_sink_lock` et `table_set_multiball`.

Le compteur est une réservation des billes attendues, pas un inventaire garanti
des sprites actuellement visibles : les callbacks des sinks peuvent attendre
si leur zone de sortie est occupée. Une assertion « trois billes actives juste
après la commande » serait donc un mauvais test. La remise à zéro des locks
est avant l'appel multibille dans la référence, après l'appel dans le fork ;
aucun effet observable de cet ordre n'est établi ici.

Reproduction, avec `pefile` et `capstone` disponibles :

```powershell
python tools/inspect_ft_multiball.py <référence-WIN95>/CADET.EXE --dependencies <audit>/tools/python --output evidence.json
```

L'allumage simultané des trois lampes reste inchangé, conformément à la
[matrice #22](behavior-comparison-matrix.md). L'écart entry lanes #33 est différé.

## Tests exécutés

Avec Zig 0.13.0, sans `NDEBUG` :

```powershell
python tools/test_ball_collision.py --cxx <outils>/zig.exe
python tools/test_multiball_routing.py --cxx <outils>/zig.exe
```

Les deux commandes réussissent. La seconde rejoue également les tests de
missions/drain existants dans les deux profils.

Le premier test compile les corps de méthodes **extraits du code de production**,
ainsi que les vraies fonctions de normalisation, produit scalaire et intersection
rayon/cercle. Les structures géométriques viennent du vrai `maths.h`. Seule la
construction graphique de TBall est remplacée par un petit état de test. Il
vérifie les collisions frontales et obliques, une cible immobile, deux billes
immobiles, l'échange des composantes normales et la conservation des composantes
tangentielles, de l'impulsion et de l'énergie à la tolérance numérique retenue.
Il vérifie aussi la distance de contact, la bille la plus proche, les exclusions
de soi-même, des billes inactives et des couches disjointes, la priorité d'un mur
plus proche, un rayon opposé et un rayon qui manque la bille.

Ce n'est **pas** une preuve de conformité au solveur FT original : le filtre
spatial, les sous-pas, les rampes à des hauteurs différentes et le déroulement
de `timed_frame` ne sont pas exécutés par ce test isolé.

Le second test compile les vrais contrôleurs. Il vérifie le compteur de locks
0→1→2→0, les demandes de remplacement, les trois commandes sink à 2 s et le
refus de recommencer une multibille déjà active. En fournissant les compteurs
après décrément du drain, il vérifie 3→2→1 sans perte de bille de réserve ni
changement de joueur, puis la dernière perte avec passage au joueur suivant.
Les sons, lampes et messages sont des doubles observables ; les remplacements
et décréments sont injectés. Ce test ne prouve donc pas le bon déclenchement
réel de `TDrain::Collision`, des timers ou des éjections.

## Couverture avant l'extension d'intégration

| Domaine | Preuve disponible | Comparaison restante |
| --- | --- | --- |
| Lanceur faible/maximal/automatique | #26, commit `4db170c` : six replays FT à 120 Hz avant/après correction XP, trajectoires et états du ressort identiques ; forces 3,90625 / 50 / 50 | Même entrée sur la référence historique ; les traces du fork seules ne prouvent pas une identité |
| Flippers, tenue et angles | Ancienne appréciation de fluidité par l'utilisateur ; #25 démontre l'invariance à différentes cadences de rendu | Entrées/sorties et tenue sur chaque flipper contre FT, après les modifications actuelles |
| Collision bille-bille | Tests numériques des méthodes de production réussis | Contacts au cours d'une vraie multibille, ordre des sous-pas et comparaison historique |
| Locks et lancement trois billes | Instructions FT authentifiées + routage source exécuté | Captures et trois éjections effectives ; sortie occupée et reprise après attente |
| Pertes successives | Routage source 3→2→1→0 exécuté | Vrai drain, timers, sons, lampes, retour à une bille dans les deux exécutables |
| Rampes et trous | Sources TRamp/THole présentes ; aucune divergence établie par cet audit | Entrée/sortie, altitude, capture et gravité, sans déduire une fidélité de la présence du code |
| Captures et éjections | TSink diffère la sortie occupée de 0,5 s ; aucune comparaison historique de ce callback ici | Sink, kickout et trou dans les deux exécutables, vitesses et délais |
| Nudges et tilt | Aucune mesure historique supplémentaire | Même instant/direction d'entrée, impulsion et seuil tilt, y compris multibille |

Pour la suite, journaliser chaque bille par identifiant, position, vitesse,
masque de collision et composant capturant, ainsi que les compteurs actif/locks,
au même pas de simulation. Associer chaque capture à un événement effectif de
sink/drain plutôt qu'à une modification directe du compteur. La référence
historique doit être mesurée séparément avec ses incertitudes de temps : ne pas
lui attribuer la cadence fixe 120 Hz du fork. L'utilisateur pourra ensuite
valider les sensations sur une version identifiée. Son retour antérieur sur
la fluidité des flippers n'est pas une validation globale de cette story.

## Extension : vrai moteur, composants et timers chargés

Deux exécutions supplémentaires de 18 s ont chargé le DAT FT authentifié, avec
le vrai `winmain`, les vrais constructeurs, la boucle physique et les timers,
sous SDL `dummy` / rendu logiciel. Il n'y a ni fenêtre visible, ni doublure des
composants ou du moteur, ni lecture audio. Les deux traces comportent chacune
2160 pas à 120 Hz et 3599 états de billes ; leurs octets sont identiques.
Les [résultats et traces](comparison-runs/multiball-issue27-2026-10-17/summary.json)
enregistrent l'identité du DAT et de l'exécutable d'audit.

Le scénario injecte des collisions à l'entrée des vrais composants ; il ne
prétend pas que ces collisions ont été atteintes par une trajectoire naturelle.
Il prépare l'éligibilité au lock (`lite4`), appelle `TSink::Collision`, puis
laisse le contrôleur et les vrais timers créer et éjecter les billes. Il n'écrit
pas directement les compteurs de locks ou de multibille.

| Temps simulé constaté | Vérification réussie |
| --- | --- |
| 4,008 puis 7,008 s | Captures réelles, locks 1 puis 2 ; chaque remplacement a été alimenté par le timer du lanceur avant la capture suivante |
| 10,008 s | Troisième capture, zéro bille active et trois réservées pour les éjections, locks remis à zéro |
| 12,108 s | Trois billes réellement actives, vitesses finies non nulles après les trois timers sink ; nudge droit appliqué aux trois vitesses (+1 ; +0,5) |
| 12,208 puis 12,308 s | Vraies collisions drain : deux puis une bille active, sans consommer la réserve de trois |
| 12,408 s | Dernier drain : aucune bille active, réserve passée à deux |
| 15,383 s | Le timer de drain puis celui du lanceur ont réalimenté une bille ; timer d'annulation du nudge effectué |
| 16,008 / 16,058 / 16,208 s | Vrai kickout : capture, maintien avant échéance, puis éjection par son timer de 100 ms |
| 16,408 puis 17,508 s | Vrai trou : capture, passage par timer et gravité, libération et changement de masque de collision |

Le test du kickout sélectionne temporairement la branche tilt de capture pour
avoir une échéance courte explicite ; il ne valide pas les récompenses ni les
temporisations ordinaires de tous les kickouts. Le test du trou place d'abord la
bille deux unités au-dessus de la hauteur de sortie. La vérification à 17,508 s
est une borne après libération, pas la durée exacte de sa chute. Les éventuels
sons et images n'ont pas été validés par ce rendu sans fenêtre.

Une première tentative du harnais vérifiait le remplacement final trop tôt :
le délai du DAT du drain doit précéder les 0,96 s du lanceur. L'attente est
désormais calculée depuis `Drain->TimerTime`, avec une marge d'échantillonnage.
Il ne s'agissait pas d'une anomalie du jeu. Aucun correctif gameplay n'a été
nécessaire pour faire réussir cette campagne.

Reproduction depuis une compilation `-CadenceAudit` :

```powershell
python tools/run_multiball_audit.py --exe <audit>/SpaceCadetPinball.exe --data <référence-WIN95>/CADET --output <nouveau-dossier> --runs 2
```

Le script impose SDL dummy, désactive l'audio, isole les préférences, exige la
fin des 13 étapes et compare les traces. Les contrôles C++ utilisent une sortie
d'erreur explicite, donc restent actifs dans la compilation `-DNDEBUG`. Le
scénario est inclus uniquement par le chemin de compilation d'audit existant.

Cette extension couvre donc désormais le fonctionnement intégré des locks,
trois éjections, drains, retour à une bille, impulsion nudge, capture kickout et
trou. Elle ne lève pas les réserves sur la fidélité historique des trajectoires,
la collision bille-bille dans la boucle complète, les rampes, les flippers et
les sensations. Les attentes de compteurs/locks sont corroborées par le binaire
FT cité plus haut ; les détails numériques de nudge et la branche courte de
kickout restent ici des vérifications du fork, pas des comparaisons historiques.
