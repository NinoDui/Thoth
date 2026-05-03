#pragma once

#include <QObject>
#include <QString>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "thoth/ContentProvider.h"
#include "thoth/Entity.h"

class Q_ASRController;
class TextParser;

class AudioContentProvider : public QObject, public IContentProvider {
    Q_OBJECT
   public:
    explicit AudioContentProvider(Q_ASRController* asrController, QObject* parent = nullptr);
    ~AudioContentProvider() override;

    void load(const std::filesystem::path& audioPath,
              std::function<void(Session)> onReady) override;
    void loadFromText(std::string_view text, std::function<void(Session)> onReady) override;
    void prepareAudio(Sentence& sentence,
                      std::function<void(bool success, const QString& errorMsg)> callback) override;

    void dumpState() const override;

   protected:
    std::string prefix() override;

   private slots:
    void onTranscriptSegmentsReady(std::vector<TranscriptSegment> segments);
    void onAsrError(const QString& errorMsg);

   private:
    std::vector<Sentence> buildSentences(const std::vector<TranscriptSegment>& segments,
                                         const std::filesystem::path& audioPath);
    void clearPending();

    Q_ASRController* m_asrController;
    std::unique_ptr<TextParser> m_textParser;

    std::filesystem::path m_pendingAudioPath;
    std::function<void(Session)> m_pendingCallback;
};
