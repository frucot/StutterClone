#pragma once

#include <juce_core/juce_core.h>

#include <functional>

class UpdateChecker
{
public:
    struct Result
    {
        juce::String latestTag;
        juce::String latestVersion;
        bool updateAvailable = false;
    };

    using Callback = std::function<void (Result)>;

    static constexpr const char* releasesPageUrl = "https://github.com/frucot/StutterClone/releases";

    static void startCheck (Callback onResult);
    static void dismissTag (const juce::String& tag);
    static void openReleasesPage();

    static bool isNewerThanCurrent (const juce::String& remoteTag);
    static juce::String displayVersionFromTag (const juce::String& tag);

private:
    struct State
    {
        juce::String lastFetchedTag;
        juce::String dismissedTag;
        juce::int64 lastCheckMs = 0;
    };

    static juce::File stateFile();
    static State loadState();
    static void saveState (const State& state);
    static juce::String fetchLatestTag();
    static void notifyIfNeeded (const Callback& callback, const juce::String& tag, const juce::String& dismissedTag);
};
