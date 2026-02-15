#include <gtest/gtest.h>
#include <config.h>
#include <fstream>
#include <filesystem>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        testConfigFile = std::filesystem::temp_directory_path() / "ark_test_config";
        std::filesystem::remove(testConfigFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(testConfigFile);
    }
    
    std::filesystem::path testConfigFile;
};

TEST_F(ConfigTest, DefaultConfigIsEmpty) {
    Config config;
    EXPECT_EQ(config.getUserName(), "");
    EXPECT_EQ(config.getUserEmail(), "");
    EXPECT_FALSE(config.hasUserConfig());
}

TEST_F(ConfigTest, SetUserName) {
    Config config;
    config.setUserName("Test User");
    EXPECT_EQ(config.getUserName(), "Test User");
}

TEST_F(ConfigTest, SetUserEmail) {
    Config config;
    config.setUserEmail("test@test.com");
    EXPECT_EQ(config.getUserEmail(), "test@test.com");
}

TEST_F(ConfigTest, HasUserConfig) {
    Config config;
    config.setUserName("Test");
    config.setUserEmail("test@test.com");
    EXPECT_TRUE(config.hasUserConfig());
}

TEST_F(ConfigTest, HasUserConfigPartial) {
    Config config;
    config.setUserName("Test");
    EXPECT_FALSE(config.hasUserConfig());
}

TEST_F(ConfigTest, SaveAndLoadConfig) {
    Config config;
    config.setUserName("SaveTest");
    config.setUserEmail("save@test.com");
    
    std::ofstream out(testConfigFile);
    out << "[user]\n";
    out << "name = SaveTest\n";
    out << "email = save@test.com\n";
    out.close();
    
    Config loadedConfig;
    std::ifstream in(testConfigFile);
    EXPECT_TRUE(in.good());
}

TEST_F(ConfigTest, LoadFromFile) {
    std::ofstream out(testConfigFile);
    out << "[user]\n";
    out << "name = FileTest\n";
    out << "email = file@test.com\n";
    out.close();
    
    Config config;
    EXPECT_EQ(config.getUserName(), "");
    EXPECT_EQ(config.getUserEmail(), "");
}
