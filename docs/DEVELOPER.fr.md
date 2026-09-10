# Guide développeur

Document pour compiler, déboguer ou étendre StutterClone. Pour jouer du plugin, voir le [manuel utilisateur](USER_MANUAL.fr.md). Pour l'installation courte, voir [README.fr.md](../README.fr.md). English: [DEVELOPER.md](DEVELOPER.md).

## Organisation du dépôt

```
CMakeLists.txt          Build, version, métadonnées du plugin JUCE
Source/
  PluginProcessor.*     Callback audio, MIDI, quantize, snapshots RT des presets
  PluginEditor.*        Interface principale
  Action.h              Actions POD, divisions, lecture des courbes
  PresetBank.*          XML (Classic d'usine + fichiers utilisateur)
  DspChain.*            Filtre → lo-fi → delay → reverb
  ActionEditor.*        Fenêtre de courbes par note
  CurveLane.*           Widget de pas
  NoteKeyboard.*        Clavier C3–B3
  WaveformDisplay.*     Vue du buffer circulaire
  Version.h.in          En-tête de version généré
.github/workflows/      CI (VST3 Windows + Linux)
```

## Numéros de version

Une seule source de vérité dans `CMakeLists.txt` :

1. `project(StutterClone VERSION 0.0.5 …)` — semver utilisé par CMake et JUCE (`JucePlugin_Version`).
2. `STUTTERCLONE_VERSION_STRING` (`"0.0.5-beta"`) — chaîne affichée dans l'éditeur et la doc.

CMake génère `build/generated/Version.h` à partir de `Source/Version.h.in`. Inclure `"Version.h"` pour la chaîne d'affichage.

Pour une release :

1. Incrémenter les deux valeurs ensemble (par exemple `0.0.5` / `"0.0.5-beta"`).
2. Ajouter une entrée dans [CHANGELOG.md](../CHANGELOG.md).
3. Commit, tag (`git tag v0.0.5-beta`), pousser le tag.

## Règles temps réel

Le thread audio (`processBlock`) ne doit pas :

- allouer (`new`, `malloc`, `std::vector::resize`, `std::function` capturante)
- prendre un verrou (`std::mutex`, `juce::CriticalSection`)
- faire de l'E/S (`DBG`, `std::cout`, disque)

Allouer les objets DSP et les buffers dans `prepareToPlay`. L'UI copie les douze `Action` dans un double buffer, puis publie en inversant un index `std::atomic`. Les wraps de boucle utilisent un court crossfade (~3 ms) anti-clic.

## Architecture audio / MIDI

1. L'audio entrant est toujours écrit dans un ring buffer de 4 secondes.
2. Un Note On sur C3–B3 arme une gesture en attente (ou démarre tout de suite si Quantize = `None`).
3. Au prochain tick de grille PPQ, le plugin capture les N derniers samples, lance le stutter, et lit les courbes de la note sur 4 beats (une mesure en 4/4), en boucle tant que la note est tenue.
4. `GestureDspChain` ne tourne que pendant la gesture (ou son fade-out).

Les notes hors C3–B3 sont ignorées. Si plusieurs notes de gesture sont tenues, la plus haute gagne.

Les courbes d'action ne sont **pas** des paramètres APVTS (cela exploserait le nombre de paramètres hôte). Elles vivent dans un `ValueTree` sauvé à côté de l'APVTS dans `getStateInformation`.

## Ajouter un paramètre courbé

1. Ajouter une valeur à `stutter::Curve` dans `Action.h` et mettre à jour `numCurves`.
2. Lui donner un id XML, une valeur par défaut, et une lecture dans `evaluateAction`.
3. La mapper dans `PresetBank.cpp` ; l'éditeur crée une `CurveLane` par `Curve`.
4. Consommer la valeur évaluée dans `PluginProcessor::processFxSlice` ou la boucle stutter.

Garder des structures POD à taille fixe pour des copies sans allocation.

## Build local

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTUTTERCLONE_COPY_PLUGIN_AFTER_BUILD=OFF
cmake --build build --target StutterClone_Standalone --parallel
```

Cibles utiles : `StutterClone_Standalone`, `StutterClone_VST3`, `StutterClone_AU` (macOS).

Les paquets Linux de la CI sont listés dans `.github/workflows/build.yml`.

## Tests et CI

Pas encore de suite de tests unitaires. GitHub Actions compile le VST3 Release sous Windows et Ubuntu. macOS/AU n'est pas dans la CI : tester l'AU en local avant une release.

## Style

- C++20, API JUCE 8 uniquement (pas de Win32 / Cocoa / CoreAudio brut).
- Préférer `noexcept` sur les helpers du thread audio.
- ASCII dans les littéraux d'interface (les constructeurs `const char*` de JUCE ne gèrent pas l'UTF-8 de façon fiable dans tous les hôtes).
