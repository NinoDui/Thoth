#include <iostream>

#include "core/gcp.h"

/**
 * @brief Test the GCP Text-to-Speech API.
 * Integration test, as mocking GCP dependencies is not economic enough for robust functionality.
 */
int main(int argc, char* argv[]) {
    GCPTextToSpeechClient ttsClient;
    auto audioData = ttsClient.execute("Hello, world!");
    std::cout << "Audio data size: " << audioData.size() << std::endl;
}