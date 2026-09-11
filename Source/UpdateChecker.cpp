#include "UpdateChecker.h"
#include "Version.h"

#include <juce_events/juce_events.h>

namespace
{
    constexpr auto kLatestApiUrl = "https://api.github.com/repos/frucot/StutterClone/releases/latest";
    constexpr juce::int64 kMinIntervalMs = 6 * 60 * 60 * 1000;
    constexpr int kTimeoutMs = 5000;

    juce::CriticalSection& stateLock()
    {
        static juce::CriticalSection lock;
        return lock;
    }

    struct ParsedVersion
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        bool preRelease = false;
    };

    ParsedVersion parseVersion (juce::String text)
    {
        text = text.trim();

        if (text.startsWithChar ('v') || text.startsWithChar ('V'))
            text = text.substring (1);

        const int dash = text.indexOfChar ('-');
        const bool preRelease = dash >= 0;

        if (preRelease)
            text = text.substring (0, dash);

        const auto parts = juce::StringArray::fromTokens (text, ".", "");
        ParsedVersion version;
        version.major = parts.size() > 0 ? parts[0].getIntValue() : 0;
        version.minor = parts.size() > 1 ? parts[1].getIntValue() : 0;
        version.patch = parts.size() > 2 ? parts[2].getIntValue() : 0;
        version.preRelease = preRelease;
        return version;
    }

    int compareVersions (const ParsedVersion& a, const ParsedVersion& b)
    {
        if (a.major != b.major)
            return a.major < b.major ? -1 : 1;

        if (a.minor != b.minor)
            return a.minor < b.minor ? -1 : 1;

        if (a.patch != b.patch)
            return a.patch < b.patch ? -1 : 1;

        if (a.preRelease != b.preRelease)
            return a.preRelease ? -1 : 1;

        return 0;
    }
}

juce::String UpdateChecker::displayVersionFromTag (const juce::String& tag)
{
    auto text = tag.trim();

    if (text.startsWithChar ('v') || text.startsWithChar ('V'))
        text = text.substring (1);

    return text;
}

bool UpdateChecker::isNewerThanCurrent (const juce::String& remoteTag)
{
    if (remoteTag.trim().isEmpty())
        return false;

    return compareVersions (parseVersion (remoteTag), parseVersion (STUTTERCLONE_VERSION_STRING)) > 0;
}

juce::File UpdateChecker::stateFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("StutterClone");
    dir.createDirectory();
    return dir.getChildFile ("update-check.xml");
}

UpdateChecker::State UpdateChecker::loadState()
{
    const juce::ScopedLock lock (stateLock());
    State state;
    auto xml = juce::XmlDocument::parse (stateFile());

    if (xml == nullptr || xml->getTagName() != "updateCheck")
        return state;

    state.lastFetchedTag = xml->getStringAttribute ("lastFetchedTag");
    state.dismissedTag = xml->getStringAttribute ("dismissedTag");
    state.lastCheckMs = xml->getStringAttribute ("lastCheckMs").getLargeIntValue();
    return state;
}

void UpdateChecker::saveState (const State& state)
{
    const juce::ScopedLock lock (stateLock());
    juce::XmlElement xml ("updateCheck");
    xml.setAttribute ("lastFetchedTag", state.lastFetchedTag);
    xml.setAttribute ("dismissedTag", state.dismissedTag);
    xml.setAttribute ("lastCheckMs", juce::String (state.lastCheckMs));

    const auto file = stateFile();
    auto temp = file.getSiblingFile (".update-check.xml.tmp");

    if (xml.writeTo (temp))
        temp.moveFileTo (file);
    else
        temp.deleteFile();
}

juce::String UpdateChecker::fetchLatestTag()
{
    juce::URL url (kLatestApiUrl);
    const auto headers = juce::String ("User-Agent: StutterClone/")
                       + STUTTERCLONE_VERSION_STRING
                       + "\r\nAccept: application/vnd.github+json\r\n";
    const auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                             .withExtraHeaders (headers)
                             .withConnectionTimeoutMs (kTimeoutMs)
                             .withNumRedirectsToFollow (5);

    auto stream = url.createInputStream (options);

    if (stream == nullptr)
        return {};

    const auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
    auto* object = parsed.getDynamicObject();

    if (object == nullptr)
        return {};

    return object->getProperty ("tag_name").toString().trim();
}

void UpdateChecker::notifyIfNeeded (const Callback& callback,
                                    const juce::String& tag,
                                    const juce::String& dismissedTag)
{
    if (callback == nullptr || tag.isEmpty() || tag == dismissedTag || ! isNewerThanCurrent (tag))
        return;

    if (juce::MessageManager::getInstanceWithoutCreating() == nullptr)
        return;

    Result result;
    result.latestTag = tag;
    result.latestVersion = displayVersionFromTag (tag);
    result.updateAvailable = true;

    juce::MessageManager::callAsync ([callback, result] { callback (result); });
}

void UpdateChecker::startCheck (Callback onResult)
{
    if (onResult == nullptr)
        return;

    const auto state = loadState();
    const auto now = juce::Time::currentTimeMillis();
    const bool throttled = state.lastCheckMs > 0 && (now - state.lastCheckMs) < kMinIntervalMs;

    if (throttled)
    {
        notifyIfNeeded (onResult, state.lastFetchedTag, state.dismissedTag);
        return;
    }

    juce::Thread::launch ([callback = std::move (onResult), state, now]
    {
        const auto tag = fetchLatestTag();
        auto next = state;
        next.lastCheckMs = now;

        if (tag.isNotEmpty())
            next.lastFetchedTag = tag;

        saveState (next);
        notifyIfNeeded (callback, next.lastFetchedTag, next.dismissedTag);
    });
}

void UpdateChecker::dismissTag (const juce::String& tag)
{
    if (tag.isEmpty())
        return;

    auto state = loadState();
    state.dismissedTag = tag;
    saveState (state);
}

void UpdateChecker::openReleasesPage()
{
    juce::URL (releasesPageUrl).launchInDefaultBrowser();
}
