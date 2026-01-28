#include <iostream>

#include "core/gcp.h"

int main(int argc, char* argv[]) {
    GCPTextToSpeechClient ttsClient;
    auto audioData = ttsClient.execute("Hello, world!");
    std::cout << "Audio data size: " << audioData.size() << std::endl;
}