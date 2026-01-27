#include <iostream>

#include "core/Client.h"
#include "core/Entity.h"

int main(int argc, char* argv[]) {
    GoogleTTSClient ttsClient;

    GoogleTTSConfig cfg;
    cfg.loadFromFile("config/tts.json");

    GoogleTTSRequest req(std::make_shared<GoogleTTSConfig>(cfg));
    req.setText("Hello, world!");

    auto result = ttsClient.execute(req);
    const auto* ttsResult = dynamic_cast<GoogleTTSResult*>(result.get());

    if (ttsResult->hasAudioData()) {
        std::cout << "Success: " << ttsResult->getAudioData().size() << std::endl;
    } else {
        std::cout << "Error: no audio data, error is " << ttsResult->getErrorMessage() << std::endl;
    }
}