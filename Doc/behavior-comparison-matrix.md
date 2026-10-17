# Matrice de fidélité Windows XP / Full Tilt WIN95

Inventaire de preuves de [#22](https://github.com/boisdal/SpaceCadetPinball/issues/22),
établi le 17 octobre 2026 sur `0a91c74` (après #25, avant les corrections #23/#24).
Les statuts ci-dessous qualifient une **affirmation**, jamais la fidélité entière
d'une édition. Le tracker conserve l'avancement des corrections.

- **Confirmé statique** : chemin du code ou instructions de la référence identifiée ; ne prouve pas le déroulement d'une partie.
- **Confirmé observé** : résultat effectivement vu, avec protocole et limites.
- **À vérifier — supposé** : piste motivée, sans divergence observable établie.
- **À vérifier — non mesuré** : domaine identifié mais sans comparaison suffisante.
- **Différence acceptée** : choix explicitement admis, avec portée. Une correction différée n'est pas une différence historique acceptée.

## Sources et identité

**R1** : [manifestes #20](reference-inventory.md). XP EXE SHA-1
`2a5b525e0f631bb6107639e2a69df15986fb0d05` ; FT WIN95 EXE SHA-1
`3f7b5699074b83fd713657cd94671f2156dbedc4`. Aucune conclusion ici ne s'étend à WIN31.

**R2** : [extraction statique du 17 octobre](comparison-runs/2026-10-17-ft-rule-evidence.json),
produite directement depuis R1 avec [inspect_ft_rule_evidence.py](../tools/inspect_ft_rule_evidence.py).
La table de dispatch à `0x42a5d4` identifie 33 états ; les noms sont rapprochés
du dispatch source, pas lus dans des symboles historiques. Les 16 appels replay
sont réextraits du binaire R1, pas simplement recopiés de l'ancien audit.

L'ancien audit local utilisait un EXE décompressé SHA-1
`cff7a1f01a039f24260cd9e9384924ab10b19878`. La comparaison des sections PE révèle
que `.text` (200 704 octets), `.rdata` et `.data` ont les mêmes adresses relatives
et les mêmes octets que R1. Elle authentifie la réutilisation des adresses et des
constats de code ci-dessous. `.idata` et `.rsrc` diffèrent, et `.reloc` n'existe
que dans R1 : ne pas affirmer une identité globale des exécutables, ressources
ou traductions. Les fichiers propriétaires et désassemblages restent locaux.

**R3** : [control.cpp](../SpaceCadetPinball/control.cpp),
[TPinballTable.cpp](../SpaceCadetPinball/TPinballTable.cpp), sources à `0a91c74`.
Les fonctions citées permettent de retrouver les observations après déplacement
des lignes. Un comportement présent côté fork n'est pas automatiquement validé
contre l'original XP.

**R4** : [observations #21](comparison-scenarios.md) et
[relevé structuré](comparison-runs/2026-10-10.json). Les scores ponctuels différents
ne constituent pas des différences de barème. La validation humaine des flippers
porte sur le fonctionnement et le ressenti de la version alors testée.

**R5** : [cadence #25](simulation-cadence.md). L'invariance des trajectoires du fork
avec FPS/taille/DPI ne démontre pas leur identité aux trajectoires historiques.

## Matrice des règles et effets

| Domaine / scénario isolé | Windows XP : référence et fork | Full Tilt WIN95 : référence et fork | État, preuve et limite | Suivi |
| --- | --- | --- | --- | --- |
| Sélection d'une mission | Fork : récompense issue de `mission_select_scores` selon mission. Original : barèmes non mesurés ici. | Fork : 100 000 à l'acceptation (`SelectMissionController`). Audit initial signale une différence déjà portée. | Confirmé statique côté fork R3 ; comparaison complète R1 non mesurée. Ne pas uniformiser les éditions. | #22 |
| Fin de mission : replay | Le fork n'appelle pas le replay dans les 16 fins listées ci-dessous. Cette matrice ne prouve pas l'absence dans toutes les éditions Windows. | R1 appelle `0x422ce0` dans 16 contrôleurs ; appels absents du fork à la base inspectée. | **Écart confirmé statique**, R2/R3. Message `-1.0` ; activation de `lite199`. Atteignabilité et consommation en jeu restent à vérifier. | #23 |
| Phases intermédiaires / simple lancement | Ne pas ajouter de récompense Windows par propagation d'un correctif FT. | Aucun appel direct replay dans les états intermédiaires listés ; attente/déploiement et sélection exclus. | Confirmé statique R2 ; ne prouve pas l'absence de toute récompense indirecte. | #23 |
| Exception Time Warp | Fork : récompense via `SpecialAddScore(2000000)` (sans ajout du jackpot) ; pas de replay de fin. | État 24 : aucun appel replay entre `0x429420` et `0x4296a0`, contrairement aux 16 autres fins. | **Exception confirmée statique R2**, à préserver dans cette référence. Ne pas appliquer la proposition upstream #240 à l'identique. | #23 |
| Récompenses / jackpot de mission | `SpecialAddScore` désactive provisoirement multiplicateur et accumulations bonus/jackpot ; pas d'ajout jackpot Windows. | Même helper ajoute le jackpot, puis le remet à 500 000. Practice : ancien audit `0x426f33` / `0x426f86`, code identique dans R1. | Confirmé statique pour helper et exemple Practice ; valeurs et conditions de toutes les missions non vérifiées dynamiquement. | #22 |
| Plafond du jackpot | Fork : 5 000 000 dans `TPinballTable::AddScore`. | Fork : 10 000 000 ; initialisation à 500 000 en FT. | Différence du code confirmée R3 ; plafonds historiques aux limites non mesurés ici. | #22 |
| Hyperspace, deuxième récompense (`activeCount == 1`) | Fork paie le jackpot, puis remet à 20 000. | Fork double le jackpot, plafonné à 10 000 000, sans paiement de cette branche. | Confirmé statique R3 ; comparaison dynamique des scores/messages avant/après non mesurée. | #22 |
| Hyperspace, autres niveaux et super-récompense | Fork gère points, bloqueur, extra-ball, Gravity Well et sons selon états. | Branches communes et branche multibille présentes. | À vérifier — non mesuré : chaque niveau, super-récompense et états combinés. `HyperspaceKickOutControl` R3 est une carte de tests, pas un résultat. | #22 |
| Lock 1, lock 2, départ multibille | Hors profil multibille normal ; préserver les règles Windows. | `table_bump_ball_sink_lock` compte les locks ; `table_set_multiball` allume ensemble `lite38/39/40`. R1 routines `0x422f30` et `0x422d20` correspondent au constat source. | Confirmé statique, **aucun écart établi** pour allumage progressif. Upstream #236 est une question, pas une preuve. Séquence de trois billes non observée ici. | #27 |
| Pertes successives en multibille | Pas de conclusion de fidélité déduite du code partagé. | `BallDrainControl` éteint certaines lampes selon `MultiballCount` ; retour à une bille à confronter. | À vérifier — non mesuré : billes restantes, lampes, timer, musique et retour au jeu simple. | #27 |
| Replay consommé / redeploy | Fork distingue `lite200` (redeploy) de `lite199` (replay). | Le drain éteint replay, active redeploy et conserve le tour par cette branche. | Confirmé statique R3 uniquement ; enchaîner gain replay → drain → drain pour comparer compteurs et messages originaux. | #23 |
| Extra balls | Fork décrémente `ExtraBalls` au drain sans changer de joueur dans cette branche. | Même branche ; ce compteur est distinct de replay et redeploy. | À vérifier — non mesuré contre R1 : gain, cumul, consommation et fin de partie. | #22 |
| Tilt | Fork annule `lite199/lite200` au drain en tilt ; pas de paiement bonus dans la branche inclinée. | Même code ; vérifier l'interaction replay/multibille. | Confirmé statique R3, conformité historique non mesurée. Ne pas déduire les effets de tous les timers de ce seul contrôle. | #23 / #27 |
| Bonus / bonus hold | `BallDrainControl` paie bonus hors tilt ; remet à 25 000 sauf maintien par `lite58`. | Même chemin source, contexte de points différent. | À vérifier — non mesuré : bonus acquis, multiplicateur, maintien, tilt puis drain sur chaque original. | #22 |
| Changement de joueur | `SwitchToNextPlayer` sauvegarde score, billes, extra balls, locks et jackpot puis diffuse `PlayerChanged`. | Même mécanisme ; états de lampes spécifiques aux composants. | À vérifier — non mesuré : deux joueurs, états volontairement différents, retour joueur 1. S1 ne teste qu'un joueur. | #22 / #23 |
| Lampes entry lanes | Fork tourne selon le côté du flipper ; comportement XP rapporté par l'utilisateur. | Original : droite → centre → gauche → droite pour les deux flippers ; fork tourne selon côté. | **Écart confirmé observé par l'utilisateur**, corroboré R3/R4. Ne pas étendre à l'autre groupe de lampes. | #33, explicitement différé |
| Lampes de mission, temporisations et animations | Nombreuses transitions présentes ; pas de campagne exhaustive. | Ne pas assimiler le replay et les lampes de sélection/progression. | À vérifier — non mesuré hors constats spécifiques ci-dessus. Photographier état asymétrique puis chaque événement. | #22 |
| Son des cibles de mission | Fork choisit selon `lite198` (mission allumée/clignotante). Comportement original XP non réécouté ici. | R1 `0x425272–0x4252ba` choisit `soundwave52` si 3 lampes, sinon `soundwave49D`. Fork utilise encore la condition Windows. | **Écart confirmé statique R2/R3** ; écouter les 6 cas mission active/inactive × 1/2/3 lampes et tester ressource absente. | #24 |
| Autres sons | Ressources XP identifiées ; sons activés dans S1/L1. | Ressources FT identifiées ; événements multiples possibles. | À vérifier — non mesuré : identité de chaque événement, chevauchement, volume et durée. Avoir entendu un son ne prouve pas sa correspondance. | #22 |
| Musiques | Désactivées dans R4 ; journal fork signale `timidity.cfg` absent. | Idem ; `BallDrainControl` demande Track1 au retour simple et après tilt. | **Non vérifié**, aucun succès d'écoute allégué. Vérifier disponibilité/import puis pistes et transitions. | #22 |
| Cadence et affichage | Fork : pas fixe 120 Hz, replays invariants aux variantes testées. | Idem. | Confirmé mesuré R5 ; **différence acceptée de méthode** : cadence de référence contrôlée, sans reproduire la charge CPU ni le compteur historique. | #25 livré |
| Sensations / collisions | Portage partagé FT confirmé par historique `ba470e8`; ce fait ne prouve pas une erreur de trajectoire Windows. | Flippers validés humainement avant #25 ; collisions bille-bille non comparées. | À vérifier — supposé pour divergence Windows ; non mesuré pour détails physiques. Aucun changement de solveur justifié par cette matrice seule. | #26 / #27 |

## Les 33 états de mission

La table suivante couvre chaque état de dispatch, y compris les transitions.
« Oui » signifie un appel **direct identifié statiquement** à la routine replay,
pas une récompense reproduite en partie. Les adresses exactes, bornes inspectées
et appels sont dans R2. Le paramètre précédent est `0xbf800000` (float -1.0)
pour les 16 appels ; il règle l'affichage du message, pas une expiration du replay.

| États | Contrôleurs | Appel replay FT |
| --- | --- | --- |
| 0, 1 | WaitingDeployment, SelectMission | Non |
| 2, 3, 4 | PracticeMission, LaunchTraining, ReentryTraining | Oui chacun |
| 5, 6, 7, 8, 9 | ScienceMission, StrayComet, BlackHoleThreat, SpaceRadiation, BugHunt | Oui chacun |
| 10 | AlienMenace, première phase | Non |
| 11, 12, 13, 14 | RescueMission, Satellite, Reconnaissance, DoomsdayMachine | Oui chacun |
| 15, 16 | CosmicPlague première phase, SecretMissionYellow | Non |
| 17, 18, 19 | TimeWarp première phase, Maelstrom première phase, Quote | Non |
| 20, 21 | AlienMenacePartTwo, CosmicPlaguePartTwo | Oui chacun |
| 22 | SecretMissionRed | Non |
| 23 | SecretMissionGreen | Oui |
| 24 | TimeWarpPartTwo | **Non, exception à conserver** |
| 25–30 | MaelstromPartTwo à MaelstromPartSeven | Non |
| 31 | MaelstromPartEight | Oui |
| 32 | Gameover | Non |

## Reproduire l'authentification statique

Avec les références locales et les modules Python `pefile` et `capstone` déjà
installés, depuis le dépôt :

```powershell
python tools/inspect_ft_rule_evidence.py C:/references/WIN95/CADET/CADET.EXE `
  --previous C:/audit/CADET-unpacked.exe `
  --dependencies C:/audit/tools/python
```

`--previous` est facultatif ; l'extraction primaire porte toujours sur la référence
WIN95 dont l'empreinte est imposée. Une autre empreinte est refusée avant toute
lecture des adresses. Aucun binaire n'est exécuté, modifié ou embarqué dans le
rapport. Les noms dépendent du dispatch source ; son SHA-256 est enregistré.

## Portée de livraison

L'inventaire couvre les domaines demandés et classe explicitement leurs lacunes.
Il ne certifie ni tous les barèmes ni les transitions historiques non rejouées.
#23 et #24 sont justifiés dès maintenant par des écarts de code authentifiés ;
leurs tests doivent vérifier les transitions de jeu et l'écoute comparée. #26 et
#27 peuvent préparer des scénarios et relever les comportements avec la cadence
contrôlée de #25 ; la validation finale des sensations appartient à l'utilisateur.
L'ancienne appréciation positive des flippers ne vaut pas validation de futures
modifications physiques. Aucun arbitrage produit supplémentaire n'est requis
pour continuer ces travaux. L'écart #33 reste différé comme demandé.

Cette matrice ne transforme aucune zone non mesurée en preuve de conformité et
ne crée aucune nouvelle story pour une divergence seulement supposée.
