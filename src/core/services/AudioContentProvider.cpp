#include "thoth/AudioContentProvider.h"

#include <filesystem>
#include <string>
#include <utility>

#include "internal/TextParser.h"
#include "thoth/Logger.h"
#include "thoth/Q_ASRController.h"

AudioContentProvider::AudioContentProvider(Q_ASRController* asrController, QObject* parent)
    : QObject(parent),
      m_asrController(asrController),
      m_textParser(std::make_unique<TextParser>()) {
    connect(m_asrController, &Q_ASRController::transcriptSegmentsReady, this,
            &AudioContentProvider::onTranscriptSegmentsReady);
    connect(m_asrController, &Q_ASRController::errorOccurred, this,
            &AudioContentProvider::onAsrError);
}

AudioContentProvider::~AudioContentProvider() = default;

void AudioContentProvider::load(const std::filesystem::path& audioPath,
                                std::function<void(Session)> onReady) {
    if (m_pendingCallback) {
        LOG_WARN("AudioContentProvider: load called while a transcription is already in progress");
        return;
    }

    if (!std::filesystem::exists(audioPath)) {
        LOG_ERROR("AudioContentProvider: audio file not found: {}", audioPath.string());
        onReady(Session{});
        return;
    }

    m_pendingAudioPath = audioPath;
    m_pendingCallback = std::move(onReady);

    LOG_INFO("AudioContentProvider: starting transcription of {}", audioPath.string());
    m_asrController->transcribeFile(QString::fromStdString(audioPath.string()));
}

void AudioContentProvider::loadFromText(std::string_view, std::function<void(Session)>) {
    LOG_WARN("AudioContentProvider::loadFromText is not supported");
}

void AudioContentProvider::prepareAudio(Sentence& sentence,
                                        std::function<void(bool, const QString&)> callback) {
    if (!sentence.audioRange) {
        callback(false, "Audio sentence is missing range metadata");
        return;
    }
    if (!std::filesystem::exists(sentence.localAudioPath)) {
        callback(false, QString("Source audio file not found: %1")
                            .arg(QString::fromStdString(sentence.localAudioPath.string())));
        return;
    }
    callback(true, "");
}

void AudioContentProvider::dumpState() const {
    LOG_INFO("[AudioContentProvider] pending={}", m_pendingAudioPath.string());
}

std::string AudioContentProvider::prefix() { return "[AudioContentProvider]"; }

void AudioContentProvider::onTranscriptSegmentsReady(std::vector<TranscriptSegment> segments) {
    if (!m_pendingCallback) {
        // This signal arrived after a scoring transcription — ignore
        return;
    }

    auto sentences = buildSentences(segments, m_pendingAudioPath);
    LOG_INFO("AudioContentProvider: built {} sentences from {} segments", sentences.size(),
             segments.size());

    Session session;
    session.id = m_pendingAudioPath.stem().string();
    session.title = m_pendingAudioPath.stem().string();
    session.sentences = std::move(sentences);
    session.inputMediaPath = m_pendingAudioPath;

    auto callback = std::exchange(m_pendingCallback, nullptr);
    m_pendingAudioPath.clear();
    callback(std::move(session));
}

void AudioContentProvider::onAsrError(const QString& errorMsg) {
    if (!m_pendingCallback) return;

    LOG_ERROR("AudioContentProvider: transcription error: {}", errorMsg.toStdString());
    clearPending();
}

void AudioContentProvider::clearPending() {
    m_pendingCallback = nullptr;
    m_pendingAudioPath.clear();
}

// Segment merging: accumulate Whisper segments until sentence-ending punctuation.
// Segments that internally contain multiple sentences are split proportionally by char count.
std::vector<Sentence> AudioContentProvider::buildSentences(
    const std::vector<TranscriptSegment>& segments, const std::filesystem::path& audioPath) {
    static const std::string kEnders = ".?!。？！";

    std::vector<Sentence> result;
    std::string accText;
    double accStart = 0.0;
    double accEnd = 0.0;
    bool hasAccum = false;

    auto flushAccum = [&]() {
        if (accText.empty() || !hasAccum) return;
        Sentence s;
        s.id = std::to_string(result.size());
        s.text = accText;
        s.audioRange = TimeRange{accStart, accEnd};
        s.localAudioPath = audioPath;
        result.push_back(std::move(s));
        accText.clear();
        hasAccum = false;
    };

    for (const auto& seg : segments) {
        auto subSentences = m_textParser->parseText(seg.text);

        if (subSentences.size() > 1) {
            flushAccum();

            size_t totalChars = 0;
            for (const auto& s : subSentences) totalChars += s.size();

            double segDuration = seg.endMs - seg.startMs;
            double timeOffset = seg.startMs;

            for (const auto& subText : subSentences) {
                if (subText.empty()) continue;
                double fraction = totalChars > 0 ? static_cast<double>(subText.size()) /
                                                       static_cast<double>(totalChars)
                                                 : 0.0;
                double subDuration = segDuration * fraction;

                Sentence s;
                s.id = std::to_string(result.size());
                s.text = subText;
                s.audioRange = TimeRange{timeOffset, timeOffset + subDuration};
                s.localAudioPath = audioPath;
                result.push_back(std::move(s));
                timeOffset += subDuration;
            }
        } else {
            std::string text = seg.text;
            if (!text.empty() && text.front() == ' ') text.erase(0, 1);
            if (text.empty()) continue;

            if (!hasAccum) {
                accStart = seg.startMs;
                hasAccum = true;
            }
            accText += (accText.empty() ? "" : " ") + text;
            accEnd = seg.endMs;

            if (kEnders.find(text.back()) != std::string::npos) {
                flushAccum();
            }
        }
    }

    flushAccum();
    return result;
}
