#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "thoth/ContentProvider.h"

namespace {

class FakeTTSEngine final : public thoth::ITTSEngine {
   public:
    thoth::TTSAudioResult synthesize(const std::string&) override { return {}; }
    std::string engineName() const override { return "fake"; }
    std::string cacheIdentity() const override { return "fake|identity"; }
    bool isAvailable() const override { return true; }
};

}  // namespace

TEST(TextContentProviderTest, ConstructorReadsCacheIdentityBeforeMovingEngine) {
    auto engine = std::make_shared<FakeTTSEngine>();
    const auto cacheDir = std::filesystem::temp_directory_path() / "thoth_text_provider_test";

    EXPECT_NO_THROW({
        TextContentProvider provider(engine, cacheDir);
        provider.dumpState();
    });
}
