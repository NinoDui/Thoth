#pragma once

#include <fmt/ostream.h>

#include <QString>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>

#include "thoth/Entity.h"
#include "thoth/ITTSEngine.h"

class AudioCache;
class Q_TTSDownloader;
class TextParser;

/**
 * @brief Interface for content providers
 *
 * A content provider is responsible for loading and preparing the content for playback.
 * 1. Loading the content from the input media path.
 * 2. Caching the audio files.
 * 3. Downloading the audio files if they are not cached.
 */
class IContentProvider {
   public:
    virtual ~IContentProvider() = default;
    virtual void load(const std::filesystem::path& inputMediaPath,
                      std::function<void(Session)> onReady) = 0;
    virtual void loadFromText(std::string_view text, std::function<void(Session)> onReady) = 0;
    virtual void prepareAudio(
        Sentence& sentence,
        std::function<void(bool success, const QString& errorMsg)> callback) = 0;

    friend std::ostream& operator<<(std::ostream& os, const IContentProvider& contentProvider);
    virtual void dumpState() const = 0;

   protected:
    virtual std::string prefix() = 0;
};

class TextContentProvider : public IContentProvider {
   public:
    explicit TextContentProvider(std::shared_ptr<thoth::ITTSEngine> engine,
                                 const std::filesystem::path& cacheDir);
    ~TextContentProvider() override;

    void load(const std::filesystem::path& inputMediaPath,
              std::function<void(Session)> onReady) override;
    void loadFromText(std::string_view text, std::function<void(Session)> onReady) override;
    void prepareAudio(Sentence& sentence,
                      std::function<void(bool success, const QString& errorMsg)> callback) override;

    friend std::ostream& operator<<(std::ostream& os, const TextContentProvider& contentProvider);
    void dumpState() const override;

   private:
    std::string prefix() override;

    std::string m_cacheIdentity;
    std::shared_ptr<Q_TTSDownloader> m_ttsDownloader;
    std::shared_ptr<TextParser> m_textParser;

    std::shared_ptr<AudioCache> m_audioCache;
    std::set<std::string> m_downloadingIdx;
};

template <>
struct fmt::formatter<TextContentProvider> : fmt::ostream_formatter {};
