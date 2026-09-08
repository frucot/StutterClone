# Directives de Développement JUCE / C++ Audio Temps Réel

## Règles Audio Hard-Real-Time (IMPORTANT)
1. **Thread Audio (`processBlock`) :**
   - Interdiction d'effectuer des allocations mémoire dynamiques (`new`, `malloc`, `std::vector::resize`, `std::function`).
   - Interdiction d'utiliser des verrous ou de l'E/S (E.g. `std::mutex`, `juce::CriticalSection`, accès disque, `DBG`, `std::cout`).
   - Toutes les structures de données (buffers de boucle, objets DSP) doivent être allouées dans `prepareToPlay`.

2. **Cross-Platform Compatibility :**
   - N'utiliser aucune API OS native (Win32, Cocoa, DirectSound, CoreAudio brut).
   - Utiliser exclusivement les wrappers natifs JUCE (`juce::File`, `juce::MathConstants`, `juce::AudioBuffer`, etc.).

3. **Paramètres et Automation :**
   - Utiliser exclusivement `juce::AudioProcessorValueTreeState` (APVTS) pour la gestion des paramètres.
   - Utiliser des `std::atomic<float>*` ou `juce::SmoothedValue` pour transmettre les modifications d'interface/MIDI vers le thread audio sans risque de verrouillage.

4. **Algorithmique Stutter / Buffer :**
   - Utiliser un buffer circulaire (`juce::AudioBuffer<float>`).
   - Appliquer systématiquement un léger fondu enchaîné (*crossfade* de 2 à 5 ms) lors des sauts de boucle pour éviter les clics/pops audio.