#include <gtest/gtest.h>

#include <filesystem>

#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"

namespace fs = std::filesystem;

TEST(ConfigStoreTest, GetCacheDirUsesConfiguredCacheDirectory) {
    auto& store = ConfigStore::instance();
    const fs::path configuredCacheDir =
        fs::temp_directory_path() / "ThothTests" / "configured-cache-dir";

    store.setValue(thoth::config::KEY_CACHE_DIR, configuredCacheDir.string());
    EXPECT_EQ(store.getCacheDir(), configuredCacheDir);

    store.setValue(thoth::config::KEY_CACHE_DIR, std::string());
    EXPECT_NE(store.getCacheDir(), configuredCacheDir);
}
