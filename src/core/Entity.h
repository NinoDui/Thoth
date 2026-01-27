#pragma once
#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class GoogleCloudConfig {
   public:
    virtual ~GoogleCloudConfig() = default;

    bool isValid() const { return !m_apiKey.empty() && !m_endpoint.empty(); }

    std::string getApiKey() const { return m_apiKey; }
    std::string getEndpoint() const { return m_endpoint; }
    void setApiKey(const std::string& apiKey) { m_apiKey = apiKey; }
    void setEndpoint(const std::string& endpoint) { m_endpoint = endpoint; }

    virtual void loadFromFile(const std::string& filePath) = 0;
    virtual std::string get(const std::string& key) const = 0;

   protected:
    std::string m_apiKey;
    std::string m_endpoint;
};

class GoogleTTSConfig : public GoogleCloudConfig {
   public:
    GoogleTTSConfig() = default;
    ~GoogleTTSConfig() override = default;

    // Polymorphic overrides
    bool isValid() const;
    void loadFromFile(const std::string& filePath) override;
    std::string get(const std::string& key) const override;

   private:
    std::string m_languageCode = "en-US";
    std::string m_voiceName = "en-US-Wavenet-1";
    std::string m_audioEncoding = "MP3";
};

class GoogleCloudRequest {
   public:
    explicit GoogleCloudRequest(std::shared_ptr<GoogleCloudConfig> config)
        : m_config(std::move(config)) {}
    virtual ~GoogleCloudRequest() = default;

    // Polymorphic interface
    virtual std::string toPayload() const = 0;
    virtual std::string get(const std::string& key) const = 0;

   protected:
    std::shared_ptr<GoogleCloudConfig> m_config;
};

class GoogleTTSRequest : public GoogleCloudRequest {
   public:
    explicit GoogleTTSRequest(std::shared_ptr<GoogleTTSConfig> config)
        : GoogleCloudRequest(config) {}
    ~GoogleTTSRequest() override = default;

    std::string toPayload() const override;
    std::string get(const std::string& key) const override;

    void setText(const std::string& text) { m_text = text; }
    std::string getText() const { return m_text; }

   private:
    std::string m_text;
};

// ============================================================================
// Qt-specific Request Adapter (Bridge Pattern)
// ============================================================================
class QTTSRequest {
   public:
    QTTSRequest() = default;
    explicit QTTSRequest(const QString& text) : m_text(text) {}
    ~QTTSRequest() = default;

    void from(const GoogleTTSRequest& request);
    QByteArray toPayload() const;

    // Getters/Setters
    QString getText() const { return m_text; }
    void setText(const QString& text) { m_text = text; }
    void setLanguageCode(const QString& code) { m_languageCode = code; }
    void setVoiceName(const QString& name) { m_voiceName = name; }
    void setAudioEncoding(const QString& encoding) { m_audioEncoding = encoding; }

    // Debug output
    friend QDebug operator<<(QDebug debug, const QTTSRequest& request);

   private:
    QString m_text;
    QString m_languageCode = "en-US";
    QString m_voiceName = "en-US-Wavenet-1";
    QString m_audioEncoding = "MP3";
};

class GoogleCloudResult {
   public:
    GoogleCloudResult() = default;
    virtual ~GoogleCloudResult() = default;

    virtual void parseFromResponse(const std::string& rawResponse) = 0;

    // Common properties
    void setSuccess(bool success) { m_success = success; }
    void setHttpCode(long code) { m_httpCode = code; }
    void setRawResponse(const std::string& response) { m_rawResponse = response; }
    void setErrorMessage(const std::string& error) { m_errorMessage = error; }

    long getHttpCode() const { return m_httpCode; }
    std::string getRawResponse() const { return m_rawResponse; }
    std::string getErrorMessage() const { return m_errorMessage; }

    void fail(const std::string& message) {
        m_success = false;
        m_errorMessage = message;
    }

   protected:
    bool m_success = false;
    long m_httpCode = 0;
    std::string m_rawResponse;
    std::string m_errorMessage;
};

class GoogleTTSResult : public GoogleCloudResult {
   public:
    GoogleTTSResult() = default;
    ~GoogleTTSResult() override = default;

    // Polymorphic override
    void parseFromResponse(const std::string& rawResponse) override;

    // TTS-specific interface
    std::vector<uint8_t> getAudioData() const { return m_audioData; }
    bool hasAudioData() const { return !m_audioData.empty(); }

   private:
    std::vector<uint8_t> m_audioData;
};
