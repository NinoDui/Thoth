#include "Client.h"

#include <curl/curl.h>

#include <nlohmann/json.hpp>

#include "Entity.h"

using json = nlohmann::json;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* p) {
    size_t totalSize = size * nmemb;
    auto* str = static_cast<std::string*>(p);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::unique_ptr<GoogleCloudResult> GoogleTTSClient::execute(const GoogleCloudRequest& request) {
    auto result = std::make_unique<GoogleTTSResult>();

    CURL* curl = curl_easy_init();
    if (!curl) {
        result->fail("Failed to initialize curl");
        return result;
    }

    std::string url = request.get("endpoint") + "?key=" + request.get("apiKey");
    std::string buffer;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.toPayload().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // 10s timeout

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    result->setHttpCode(httpCode);
    result->setRawResponse(buffer);

    if (res != CURLE_OK) {
        result->fail("Failed to perform curl request");
    } else if (result->getHttpCode() != 200) {
        try {
            auto errorJson = json::parse(buffer);
            if (errorJson.contains("error") && errorJson["error"].contains("message")) {
                result->fail("API Error: " + errorJson["error"]["message"].get<std::string>());
            } else {
                result->fail("HTTP Error: " + std::to_string(result->getHttpCode()));
            }
        } catch (const json::exception& e) {
            result->fail("Failed to parse JSON response: " + std::string(e.what()));
        }
    } else {
        try {
            auto jsonResponse = json::parse(buffer);
            result->parseFromResponse(jsonResponse.dump());
        } catch (const json::exception& e) {
            result->fail("Failed to parse JSON response: " + std::string(e.what()));
        }
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return result;
}