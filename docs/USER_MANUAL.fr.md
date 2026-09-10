# Manuel utilisateur StutterClone

Version **0.0.5**. English: [USER_MANUAL.md](USER_MANUAL.md).

StutterClone est un **effet audio** déclenché par **MIDI**. Ce n'est pas un synthétiseur : il traite le son déjà présent sur la piste, et une note MIDI de **C3 à B3** décide *quand* stutter et *comment*.

Ce projet est indépendant et n'est affilié à aucun produit commercial de stutter.

## Principe

L'audio entrant est toujours enregistré dans un buffer circulaire (environ quatre secondes). Quand vous tenez une note de geste, le plugin :

1. Attend la prochaine grille **Quantize** (ou démarre tout de suite si Quantize = `None`).
2. Capture un fragment dont la longueur suit la **Division** de cette note (synchronisée au tempo).
3. Remplace le son live par cette boucle tant que la note est tenue.
4. Lit l'**action** de la note sur une mesure en 4/4 (quatre temps). L'action est un jeu de courbes en pas : division du stutter, reverse, pan, filtre, lo-fi, delay et reverb.
5. Recroise vers le signal live à la relâche de la note (environ 3 ms).

Les notes hors C3–B3 sont ignorées. Si plusieurs notes de geste sont tenues en même temps, **la plus aiguë gagne**.

Le clavier à l'écran **sélectionne l'action à éditer** (cyan) et **joue ce slot tant que le clic est maintenu** (orange). Le MIDI d'un contrôleur, d'un clip ou du standalone déclenche les mêmes slots.

## Installation

| Format | Plateformes | Emplacement typique |
| --- | --- | --- |
| VST3 | macOS, Windows, Linux | macOS : `~/Library/Audio/Plug-Ins/VST3/` |
| AU | macOS | `~/Library/Audio/Plug-Ins/Components/` |
| Standalone | toutes | à côté du build, ou le bundle app sous macOS |

Après une nouvelle copie, relancez un rescan ou le DAW. Sous macOS, l'AU est un **Music Effect** (`aumf`) : les hôtes compatibles exposent une entrée MIDI sur l'effet audio.

## Flux du signal

Insérez StutterClone comme **insert** sur une piste audio (ou un groupe / retour).

- Les effets **avant** StutterClone sont capturés dans la boucle.
- Les effets **après** StutterClone traitent la sortie stutterée (ou le dry).
- Sans geste actif, l'audio traverse en dry. Le buffer continue d'enregistrer, pour que la prochaine capture ait du matériau récent.

Il faut toujours deux choses à l'insert : de l'**audio** et du **MIDI** (C3–B3).

## Routage

La façon d'envoyer du MIDI dans un effet audio dépend de l'hôte. L'exemple Ableton Live ci-dessous est le schéma à recopier ailleurs : une piste audio pour le son, une piste MIDI pointée vers le plugin.

### Ableton Live (exemple)

Utilisez **deux pistes**. Live ne joue pas de clip MIDI sur une piste audio : le MIDI doit venir d'une piste MIDI dont la sortie est le plugin.

**1. Piste audio — le son**

1. Créez une piste audio (par exemple `Vocal`).
2. Chargez un clip, ou réglez **Audio From** sur l'entrée de votre interface.
3. Dans la vue Device, insérez **StutterClone** (VST3 ou AU).
4. Placez l'EQ / compression que vous voulez *dans* la boucle **avant** StutterClone ; les effets de type send **après**.
5. Laissez **Monitor** comme d'habitude (`Auto` pour les clips, `In` pour une entrée live).

**2. Piste MIDI — les gestes**

1. Créez une piste MIDI (par exemple `Stutter MIDI`).
2. **MIDI From** : votre contrôleur, ou `All Ins`. Armez la piste (ou Monitor `In`) pour qu'un clavier arrive dans Live.
3. **MIDI To** : la piste audio (`Vocal`), puis dans le second menu choisissez **StutterClone** — pas `Track In`.
4. Dessinez ou enregistrez des notes **C3–B3** dans un clip, ou jouez-les en live. Les autres hauteurs ne font rien.

**3. Vérification**

1. Lancez le transport de Live (le plugin suit le tempo / PPQ de l'hôte).
2. Faites jouer l'audio sur `Vocal`.
3. Tenez C3. L'en-tête doit passer **Pending** (attente du Quantize) puis **Active**. La waveform montre la boucle capturée.

Si l'en-tête reste **Inactive**, le MIDI n'atteint pas le plugin : vérifiez que **MIDI To** est bien StutterClone, que la piste MIDI est armée ou qu'un clip joue, et que les notes sont en C3–B3 (le C3 d'Ableton est la note MIDI 60).

**Schéma typique**

```
[Audio : Vocal]   clip ou entrée  -->  (FX optionnels)  -->  StutterClone  -->  (FX optionnels)  -->  master
[MIDI : Gestes]   contrôleur/clip -->  MIDI To : Vocal / StutterClone
```

Vous pouvez séquencer les stutters avec un clip MIDI sur `Stutter MIDI`, ou les jouer en live depuis un contrôleur de deux octaves autour de C3.

### Autres hôtes (même idée)

- **Logic Pro** : insérer StutterClone comme FX audio ; l'AU Music Effect devrait afficher une entrée MIDI. Router un instrument ou du MIDI externe vers cette entrée.
- **Standalone** : autoriser le micro si demandé, choisir l'entrée audio dans les réglages, et jouer C3–B3 depuis un clavier MIDI.

## Interface

Le plugin s'ouvre **déplié**. **Editor** (en haut à droite) replie le panneau de courbes en vue performance compacte (en-tête, presets, waveform, clavier) et le redéplie en pleine hauteur pour afficher toutes les lanes.

### En-tête

| Contrôle | Rôle |
| --- | --- |
| Titre / version | Nom du plugin et version affichée |
| ? | Overlay court : routage, gestes, et fonctionnement des actions |
| Ligne de geste | Note en lecture ou en attente, division courante, et pas (par exemple `C3 \| 1/16 \| step 3`). Au repos : `C3-B3 \| hold a note` (la plage suit Octave - / +) |
| Badge BPM | Tempo annoncé par l'hôte |
| Badge MIDI | `Inactive` (rouge) / `Pending` (jaune, attente Quantize) / `Active` (cyan) |

### Presets et Quantize

Un **preset** est le Quantize plus les douze actions (une par note).

| Contrôle | Rôle |
| --- | --- |
| Menu preset | Charger **Classic** (usine) ou un preset utilisateur |
| Save | Écraser le preset **utilisateur** courant. Sur Classic, ouvre Save As |
| Save As | Enregistrer un nouveau preset utilisateur (overlay de nom) |
| Delete | Supprimer un preset utilisateur. Classic ne peut pas être effacé |
| Quantize | Moment où un geste peut démarrer : `1/4`, `1/8`, `1/16`, `1/32`, ou `None` (immédiat) |

Presets utilisateur sur disque :

- macOS : `~/Library/Application Support/StutterClone/Presets/`
- Windows : `%APPDATA%/StutterClone/Presets/`
- Linux : `~/.config/StutterClone/Presets/`

La session du DAW stocke aussi le preset de travail avec l'état du plugin : un projet recharge vos derniers edits même sans Save.

### Waveform et clavier

La waveform est le buffer de capture : entrée live au repos, fragment en boucle pendant un geste (tête d'écriture en jaune).

Le clavier à douze touches (défaut C3–B3 = MIDI 60–71). **Octave - / +** déplacent toute la fenêtre de 12 notes MIDI :

- Un clic sur une touche **édite** l'action de ce slot (cyan) et déplie l'éditeur s'il était replié.
- **Maintenir le clic** joue le slot ; un overlay orange montre la note en train de sonner.
- Les touches s'allument aussi en orange quand la note MIDI correspondante est tenue.
- Octave - place le slot 1 sur MIDI 48 (C2) ; Octave + sur MIDI 72 (C4). Les douze actions restent dans les mêmes slots.

### Classic d'usine (divisions par défaut)

Tant que vous ne dessinez pas de courbes, **Classic** ne change que la **Division** par note. Les autres modules restent éteints.

| Note | Division par défaut |
| --- | --- |
| C3, C#3 | 1/8 |
| D3, D#3 | 1/16 |
| E3 | 1/32 |
| F3, F#3 | 1/64 |
| G3 | 1/4 |
| G#3 | 1/8T |
| A3 | 1/16T |
| A#3 | 1/32T |
| B3 | 1/1 |

T = triolet. Les anciens presets en sextolets (`S`) se chargent comme le triolet correspondant.

## Éditeur d'action

Chaque note a sa propre action. Cliquez le clavier (ou jouez une note, puis cliquez la touche) pour l'éditer. Un playhead rouge parcourt les lanes tant que cette note est le geste actif.

Les courbes couvrent **une mesure** (quatre temps en 4/4). En mode normal elles recommencent tant que vous tenez la note. Avec **Ping-Pong**, la mesure est lue à l'endroit puis à l'envers (huit temps), puis ça recommence. L'axe horizontal est le temps ; le nombre de colonnes est **Grid**.

### Barre d'outils

| Contrôle | Rôle |
| --- | --- |
| Grid | `4`, `8`, `16` ou `32` pas sur la mesure. Changer la grille rééchantillonne les courbes existantes |
| Ping-Pong | Off (défaut) : revenir au début de la mesure. On : lire la mesure à l'endroit puis à l'envers (8 temps) |
| Unfreeze Loop | Off (défaut) : garder le fragment capturé au début du geste. On : recapturer depuis le ring à chaque période **Loop** |
| Loop | Période de recapture si Unfreeze est on : `1 Beat`, `2 Beats`, `3 Beats`, `1 Bar`, `2 Bars` |
| Delay Cut | À la relâche, vider le delay pour que les queues ne continuent pas |
| Reverb Cut | À la relâche, vider la reverb pour que les queues ne continuent pas |

Sans les options Cut, delay et reverb peuvent encore sonner pendant le court fondu vers le dry.

### Dessiner les lanes

- **Lanes continues** (Division, Cutoff, Mix, …) : clic-glissé. La hauteur est la valeur (vers le haut = plus / division plus courte).
- **Lanes on/off** (vert = On, gris = Off) : un clic inverse le pas ; un glissé peint cette valeur sur les pas suivants.
- Après un glissé, le dernier pas affiche une valeur (Hz, %, On/Off, nom de division, …).

### Stutter

| Lane | Rôle |
| --- | --- |
| Division | Longueur de boucle en temps : `1/1`, `1/2`, `1/4` … `1/64`, plus triolets (`T`). La lane est ordonnée par durée : plus long en bas, plus court en haut |
| Reverse | Lire le fragment à l'envers |
| Alt Pan | Alterner le stutter gauche / droite (le bascule est lissé pour éviter les clics) |

### Filter

Le menu **Filter** à côté du titre du groupe est le type pour cette note : **Lowpass**, **Highpass** ou **Bandpass**.

| Lane | Rôle |
| --- | --- |
| Filter On | Activer le filtre sur ce pas |
| Cutoff | Environ 20 Hz – 20 kHz |
| Resonance | Environ 0,10 – 4,00 |

### Lo-Fi

| Lane | Rôle |
| --- | --- |
| Lo-Fi On | Activer la réduction de bits et le downsampling |
| Bits | Environ 1–16 bits |
| Downsample | Facteur entier 1–16 (1 = pas de downsampling extra) |

### Delay

Le menu **Delay** à côté du titre du groupe est le temps de delay synchronisé : **1/4**, **1/8**, **1/16**, **1/32**.

| Lane | Rôle |
| --- | --- |
| Delay On | Activer le delay |
| Delay Mix | Dry/wet |
| Feedback | Répétitions (plafonné sous 1 pour ne pas s'emballer) |

### Reverb

| Lane | Rôle |
| --- | --- |
| Reverb On | Activer la reverb |
| Reverb Mix | Dry/wet |
| Size | Taille de pièce |
| Damping | Amortissement des aigus |

La chaîne d'effets est **filtre → lo-fi → delay → reverb**, et elle ne tourne que pendant un geste (ou son fade-out).

## Conseils de jeu

- Jouez ou tenez les notes dans le temps du Quantize pour caler les attaques sur la grille.
- Laissez le transport de l'hôte tourner si vous dépendez du Quantize ; tempo et PPQ viennent du DAW.
- Activez Unfreeze si la source doit continuer d'évoluer dans le stutter (pad, phrase vocale) au lieu d'un fragment figé.
- Laissez Unfreeze off pour un glitch « attraper et répéter » classique.
- Plusieurs notes superposées se résolvent toujours vers la plus aiguë ; relâchez-la pour retomber sur la suivante encore tenue, ou sur le dry s'il n'en reste aucune.

## Dépannage

| Symptôme | À vérifier |
| --- | --- |
| Pas de stutter, en-tête Inactive | Le MIDI n'arrive pas au plugin, ou les notes sont hors de la fenêtre Octave courante |
| Pending sans fin | Transport arrêté, ou la grille Quantize n'est jamais atteinte. Essayer `None` |
| Stutter sans FX | Filter On, Lo-Fi On, Delay On, Reverb On sont off sur ces pas |
| Delay ou reverb qui traîne après la relâche | Activer Delay Cut / Reverb Cut sur cette action |
| Fenêtre trop petite / lanes à scroller | Déplier **Editor** ; la fenêtre doit grandir pour tout montrer. Recharger le plugin si le DAW a mémorisé une ancienne taille |
| L'AU n'a pas d'entrée MIDI | Utiliser le build AU (Music Effect). Certains hôtes n'exposent le MIDI que sur le VST3 |

## Licence

StutterClone est un logiciel libre sous [GNU Affero General Public License v3.0](../LICENSE).
