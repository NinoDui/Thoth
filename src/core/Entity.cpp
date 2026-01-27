#include "Entity.h"

#include <fstream>
#include <nlohmann/json.hpp>

bool GoogleTTSConfig::isValid() const {
    return !m_endpoint.empty() && !m_apiKey.empty() && !m_languageCode.empty() &&
           !m_voiceName.empty() && !m_audioEncoding.empty();
}

void GoogleTTSConfig::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    nlohmann::json json;
    file >> json;

    m_apiKey = json["apiKey"].get<std::string>();
    m_endpoint = json["endpoint"].get<std::string>();
    if (json.contains("languageCode")) {
        m_languageCode = json["languageCode"].get<std::string>();
    }
    if (json.contains("voiceName")) {
        m_voiceName = json["voiceName"].get<std::string>();
    }
    if (json.contains("audioEncoding")) {
        m_audioEncoding = json["audioEncoding"].get<std::string>();
    }

    if (!isValid()) {
        throw std::runtime_error("Invalid configuration file: " + filePath);
    }
}

std::string GoogleTTSConfig::get(const std::string& key) const {
    if (key == "languageCode") {
        return m_languageCode;
    } else if (key == "voiceName") {
        return m_voiceName;
    } else if (key == "audioEncoding") {
        return m_audioEncoding;
    } else if (key == "endpoint") {
        return m_endpoint;
    } else if (key == "apiKey") {
        return m_apiKey;
    }
    throw std::runtime_error("Invalid key: " + key);
}

std::string GoogleTTSRequest::toPayload() const {
    nlohmann::json payload;

    // Input section
    payload["input"]["text"] = m_text;

    // Voice section
    payload["voice"]["languageCode"] = m_config->get("languageCode");
    payload["voice"]["name"] = m_config->get("voiceName");

    // Audio config section
    payload["audioConfig"]["audioEncoding"] = m_config->get("audioEncoding");

    return payload.dump();
}

std::string GoogleTTSRequest::get(const std::string& key) const {
    if (key == "text") {
        return m_text;
    } else {
        return m_config->get(key);
    }
}

void QTTSRequest::from(const GoogleTTSRequest& request) {
    m_text = QString::fromStdString(request.get("text"));
    m_languageCode = QString::fromStdString(request.get("languageCode"));
    m_voiceName = QString::fromStdString(request.get("voiceName"));
    m_audioEncoding = QString::fromStdString(request.get("audioEncoding"));
}

QByteArray QTTSRequest::toPayload() const {
    QJsonObject root;

    // Input section
    QJsonObject inputObj;
    inputObj["text"] = m_text;
    root["input"] = inputObj;

    // Voice section
    QJsonObject voiceObj;
    voiceObj["languageCode"] = m_languageCode;
    voiceObj["name"] = m_voiceName;
    root["voice"] = voiceObj;

    // Audio config section
    QJsonObject audioConfigObj;
    audioConfigObj["audioEncoding"] = m_audioEncoding;
    root["audioConfig"] = audioConfigObj;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QDebug operator<<(QDebug debug, const QTTSRequest& request) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "QTTSRequest("
                    << "text: " << request.m_text << ", languageCode: " << request.m_languageCode
                    << ", voiceName: " << request.m_voiceName
                    << ", audioEncoding: " << request.m_audioEncoding << ")";
    return debug;
}

void GoogleTTSResult::parseFromResponse(const std::string& rawResponse) {
    m_rawResponse = rawResponse;

    try {
        nlohmann::json jsonResponse = nlohmann::json::parse(rawResponse);

        if (jsonResponse.contains("audioContent")) {
            // Extract base64-encoded audio data
            std::string audioContent = jsonResponse["audioContent"].get<std::string>();
            // TODO: Decode base64 to binary audio data
            // For now, just store as-is
            m_audioData.assign(audioContent.begin(), audioContent.end());
            m_success = true;
        } else if (jsonResponse.contains("error")) {
            fail("API Error: " + jsonResponse["error"]["message"].get<std::string>());
        }
    } catch (const nlohmann::json::exception& e) {
        fail("Failed to parse JSON response: " + std::string(e.what()));
    }
}
