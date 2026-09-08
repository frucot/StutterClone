# StutterClone

[![Build](https://github.com/frucot/StutterClone/actions/workflows/build.yml/badge.svg)](https://github.com/frucot/StutterClone/actions/workflows/build.yml)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.0.2-cyan.svg)](CHANGELOG.md)

**Version 0.0.2** — effet stutter / glitch déclenché par MIDI, synchronisé au tempo, en VST3, AU et standalone.

[English](README.md) · [Guide développeur](docs/DEVELOPER.fr.md) · [Developer guide](docs/DEVELOPER.md)

## Présentation

StutterClone est un effet audio open source construit avec [JUCE](https://juce.com) 8. En maintenant une note MIDI de **C3 à B3**, le plugin capture l'audio en direct dans une boucle dont la longueur suit le tempo de l'hôte. Chacune des douze notes a sa propre **action** : des courbes en pas (4 / 8 / 16 / 32 sur une mesure) qui pilotent la division du stutter, le reverse, le pan, le filtre, le lo-fi, le delay et la reverb.

L'ensemble des douze actions forme un **preset**, enregistrable et rappelable depuis le plugin.

Ce projet est indépendant et n'est affilié à aucun produit commercial de stutter.

## Fonctionnalités

- Beat repeat synchronisé au tempo, divisions droites, triolets et sextolets
- Éditeur d'action par note (C3–B3) avec courbes dessinables
- Quantize de départ : `None`, `1/4`, `1/8`, `1/16`, `1/32`
- Preset d'usine **Classic** et presets utilisateur sur disque
- Affichage de la forme d'onde du buffer circulaire
- Formats : **VST3** (macOS, Windows, Linux), **AU** (macOS), **Standalone**

## Prérequis

- CMake 3.22 ou plus récent
- Un compilateur C++20 (Xcode Command Line Tools, Visual Studio 2022, ou GCC 12+)
- Sous Linux : en-têtes ALSA/JACK et les paquets GUI JUCE habituels (voir le [guide développeur](docs/DEVELOPER.fr.md))

JUCE 8.0.15 est téléchargé automatiquement par CMake (FetchContent). Pas besoin d'une copie locale de JUCE.

## Compilation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Options CMake :

| Option | Défaut | Rôle |
| --- | --- | --- |
| `STUTTERCLONE_FORMATS` | `VST3;AU;Standalone` | Formats (AU ignoré sous Windows/Linux) |
| `STUTTERCLONE_COPY_PLUGIN_AFTER_BUILD` | `ON` | Copie VST3/AU dans les dossiers plugins utilisateur |

Après un build macOS avec copie activée :

- VST3 → `~/Library/Audio/Plug-Ins/VST3/StutterClone.vst3`
- AU → `~/Library/Audio/Plug-Ins/Components/StutterClone.component`

Relancez un rescan (ou le DAW) avant de charger une nouvelle version.

## Utilisation

1. Insérer **StutterClone** comme **effet audio** sur une piste audio.
2. Router le MIDI vers le plugin (l'AU est un Music Effect, l'hôte expose une entrée MIDI).
3. Jouer ou maintenir les notes **C3–B3**. L'effet attend la prochaine grille de quantize (ou part tout de suite si Quantize = `None`), puis boucle le fragment capturé pendant une mesure tant que la note est tenue.
4. Ouvrir **Edit Action** pour dessiner les courbes de cette note. Enregistrer les douze actions comme preset.

Presets utilisateur :

- macOS : `~/Library/Application Support/StutterClone/Presets/`
- Windows : `%APPDATA%/StutterClone/Presets/`
- Linux : `~/.config/StutterClone/Presets/`

## Versions

La version publique est **0.0.2**. CMake et les métadonnées du plugin utilisent le même semver `0.0.2`. La chaîne affichée `STUTTERCLONE_VERSION_STRING` est définie dans [`CMakeLists.txt`](CMakeLists.txt) et générée dans `Version.h`. Modifier les deux au même moment pour une release. Voir [CHANGELOG.md](CHANGELOG.md).

## Contribuer

Les rapports de bugs et les pull requests sont les bienvenus. Lire [CONTRIBUTING.md](CONTRIBUTING.md) et le [guide développeur](docs/DEVELOPER.fr.md) (contraintes temps réel).

## Licence

StutterClone est un logiciel libre sous [GNU Affero General Public License v3.0](LICENSE).

Il est lié à JUCE, également disponible sous AGPLv3 pour les projets open source. Si vous distribuez une version modifiée (y compris un service hébergé qui l'exécute), vous devez fournir le code source correspondant sous AGPL.

## Sécurité

Ne déposez pas de failles en issue publique. Voir [SECURITY.md](SECURITY.md).
