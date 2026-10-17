# Préparation de contributions upstream — #16

Document de revue, préparé le 17 octobre 2026. **Aucun message, commentaire ou
PR n'a été envoyé à upstream.** Le suivi et les décisions restent dans
[#16](https://github.com/boisdal/SpaceCadetPinball/issues/16) et son parent
[#5](https://github.com/boisdal/SpaceCadetPinball/issues/5).

## Politique relue et portée de la préparation

Le remote `upstream` désigne `k4zmu2a/SpaceCadetPinball`. Après `git fetch
upstream master`, la tête consultée est
`cb9b7b886244a27773f66b0b19fdc2998392565e`.
Le [CONTRIBUTING à cette révision](https://github.com/k4zmu2a/SpaceCadetPinball/blob/cb9b7b886244a27773f66b0b19fdc2998392565e/CONTRIBUTING.md)
demande d'indiquer version et branche/release pour les problèmes, renvoie les
problèmes de ports vers leurs propres dépôts et exclut les ports supplémentaires
du dépôt principal. Il ne garantit pas l'acceptation d'une PR et conseille :
« If you are unsure, ask first, make PR second. »

Le [README relu](https://github.com/k4zmu2a/SpaceCadetPinball/blob/cb9b7b886244a27773f66b0b19fdc2998392565e/README.md)
vise Windows, Linux et macOS, conserve les ressources originales hors du dépôt
et identifie les références XP et FT par SHA-1. Ce sont les mêmes empreintes
que celles imposées par nos extracteurs.

Aucune règle explicite concernant les contributions assistées ou générées par
IA n'a été trouvée dans ces deux documents ni dans les fichiers de politique
présents à cette révision ; `.github` contient le workflow ReleaseBuilds, sans
modèle de PR ou politique IA distincte. Les recherches ciblées complémentaires
n'ont pas établi une déclaration du mainteneur sur ce sujet. Ce constat est
limité aux sources consultées : **il ne constitue ni une permission implicite,
ni une preuve d'absence de consigne ailleurs**. Il faudra relire les règles au
moment d'une proposition éventuelle et demander explicitement la préférence.

Le code, les scripts de vérification et cette rédaction ont bénéficié d'une
assistance IA substantielle, notamment par Codex. Une future proposition doit
le dire, sans présenter les résultats comme une revue humaine déjà faite.
L'auteur humain qui soumettra devra pouvoir expliquer et assumer le patch.

## Résumé factuel des candidats

La base du fork examinée pour ce dossier est `58c8fa3`. Les commits ci-dessous
permettent d'identifier les travaux ; ils ne sont pas des branches prêtes à
fusionner dans upstream. Un futur patch sera isolé sur la tête upstream alors
courante et vérifié à nouveau.

| Sujet | Changement et preuves disponibles | Limites et périmètre proposé |
| --- | --- | --- |
| Cibles de mission FT, fork #24 | Commit `8467100`, publié dans le fork par [PR #37](https://github.com/boisdal/SpaceCadetPinball/pull/37). En FT, `soundwave52` lorsque les trois lampes sont allumées après le coup ; `soundwave49D` sinon. Condition XP conservée. Instructions FT authentifiées `0x425272–0x4252ba`, tags des sons identifiés, 72 cas automatisés et garde de ressource absente. [Détail](mission-target-sounds.md). | Petit candidat historique prioritaire. [Upstream #206](https://github.com/k4zmu2a/SpaceCadetPinball/issues/206), encore ouverte lors de cette lecture, décrit ce même écart. La sélection sonore actuelle reste inchangée à la tête upstream consultée. Les doubles ne prouvent pas une écoute comparée ou le mélange sonore réel. L'interface d'import ne fait pas partie de ce patch. |
| Impulsion du lancement automatique XP, fork #26 | Commit `4db170c`, présent dans la base locale examinée. La commande du menu remet une fenêtre de force maximale de 200 ms en XP, au lieu du relâchement manuel de 25 ms ; FT et tir manuel inchangés. Référence XP authentifiée : message 1017, branche `0x1018691`, constante 0,2. Douze replays du fork avant/après : seule cette fenêtre change, 2→23 échantillons positifs. [Détail](windows-physics.md). | Petit candidat historique distinct. Le chemin de lancement upstream consulté passe toujours par le relâchement manuel. Les trajectoires des six scénarios restent identiques avant/après : ne pas promettre un correctif démontré des tirs ratés, une amélioration perceptible ou la fidélité générale du solveur. La publication de ce commit dans le fork devra être confirmée avant d'envoyer un lien public. |
| Replays de mission FT, fork #23 | Seize fins identifiées dans la référence, exception Time Warp préservée ; [preuves et tests](mission-replays.md), y compris cycle de vie avec composants/timers réels dans le fork. | [PR upstream #240](https://github.com/k4zmu2a/SpaceCadetPinball/pull/240) déjà ouverte : ne pas ouvrir automatiquement un doublon. Si le mainteneur le souhaite, proposer les preuves et la distinction Time Warp dans le cadre existant. |
| Cadence du fork, #25 | [PR #35](https://github.com/boisdal/SpaceCadetPinball/pull/35) fusionnée dans le fork, merge `0a91c74` : simulation fixe indépendante du rendu, récupération d'une dette courte après blocage. Quinze replays et traces documentés dans [simulation-cadence.md](simulation-cadence.md). | Changement d'architecture plus large, à discuter séparément. L'origine du pic historique de 118 ms n'a pas été identifiée ; le test avec blocage injecté démontre la récupération de simulation, pas la suppression du blocage visuel ni sa cause. Ne pas l'annoncer comme correctif causal du pic. |

Les preuves n'embarquent pas d'exécutable historique, de DAT, de son ou de
musique propriétaires. Les extracteurs utilisent les références locales de la
personne qui reproduit le contrôle. Les empreintes, adresses interprétées,
traces numériques et sources des tests rendent les limites vérifiables.

Les imports de ressources, la sélection persistante des modes, les records
séparés, les réglages DPI/affichage et le packaging Windows autonome répondent
aux objectifs du fork. Ils ne doivent pas être regroupés avec ces correctifs
historiques ni présentés comme demandés par upstream. Le fait que les plateformes
desktop soient déjà prises en charge n'implique pas l'acceptation de ces choix
d'interface ou de distribution.

## Question au mainteneur — brouillon anglais non envoyé

**Suggested subject:** Historical behavior fixes and AI-assisted contributions

Hello,

I am maintaining a fork based on SpaceCadetPinball and have prepared two small
historical behavior fixes against the XP and Full Tilt executable fingerprints
listed in your README:

- Full Tilt mission-target sound selection, matching the issue described in
  [#206](https://github.com/k4zmu2a/SpaceCadetPinball/issues/206), while retaining
  the Windows condition.
- The XP menu's automatic-launch impulse window: 200 ms rather than the manual
  release's 25 ms. The FT and manual-launch paths remain unchanged. Our tested
  trajectories did not change, so I am not claiming this fixes missed launches.

The investigation, code and test preparation used substantial AI assistance
(Codex). Before opening any PR, would you be willing to review narrowly scoped
contributions prepared this way, with explicit disclosure, reproducible evidence
and their limitations? If so, what form of human review and supporting tests
would you expect, and which of these changes would be useful to you?

I would keep the fork's resource-import UI, mode selection, display settings,
packaging and broader frame-scheduling changes out of these patches. I also saw
the existing mission-replay PR #240 and would not open a duplicate.

Thank you.

## Préparation avant un éventuel envoi

La question ci-dessus est prête à relire, sans engagement d'envoi. L'autorisation
de l'utilisateur pour contacter upstream reste nécessaire conformément aux
décisions de #5/#16 ; aucun silence du mainteneur ne vaut accord. La préparation
des autres travaux du fork ne dépend pas de cette décision.

Si l'utilisateur autorise le contact et que le mainteneur accepte le principe,
réexaminer les issues/PR et la politique alors actuelles, produire un patch
minimal par correction sur cette nouvelle base, vérifier ses preuves et publier
uniquement les liens effectivement accessibles. Indiquer la version testée,
les plateformes réellement compilées/exécutées et les limites ; ne pas déclarer
Linux/macOS validés sur la seule base d'une compilation Windows. Le résumé de
test doit distinguer assertions avec doubles, composants réels, comparaison du
binaire historique et appréciation humaine.
