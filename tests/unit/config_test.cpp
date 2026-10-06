#include "support/ark_test.h"

#include <ark/repo/config.h>
#include <ark/cli/commands.h>

#include <cstdlib>

using namespace arktest;

namespace {

class ConfigTest : public ::testing::Test {
protected:
  void SetUp() override {
    const char *home = std::getenv("HOME");
    if (home) {
      savedHome_ = home;
    }
    // Point $HOME at an empty dir so the developer's real ~/.arkconfig can't leak in.
    fs::create_directories(tmp_.path() / "home");
    setenv("HOME", (tmp_.path() / "home").c_str(), 1);
    fs::create_directories(tmp_.path() / "repo");
    cwd_ = std::make_unique<CwdGuard>(tmp_.path() / "repo");
    ASSERT_EQ(cmd_init({}), 0);
  }
  void TearDown() override {
    cwd_.reset();
    if (savedHome_) {
      setenv("HOME", savedHome_->c_str(), 1);
    }
  }

  Config loadRepoConfig(const std::string &text) {
    writeFile(".ark/config", text);
    Config c(repo_);
    c.load();
    return c;
  }

  fs::path home() const { return tmp_.path() / "home"; }

  TempDir tmp_;
  Repository repo_;
  std::unique_ptr<CwdGuard> cwd_;
  std::optional<std::string> savedHome_;
};

} // namespace

TEST_F(ConfigTest, ReadsUserSection) {
  Config c = loadRepoConfig("[user]\nname = Ada Lovelace\nemail = ada@example.com\n");
  EXPECT_EQ(c.getUserName(), "Ada Lovelace");
  EXPECT_EQ(c.getUserEmail(), "ada@example.com");
  EXPECT_TRUE(c.hasUserConfig());
}

TEST_F(ConfigTest, ToleratesWhitespaceTabsQuotesAndCrlf) {
  Config c = loadRepoConfig("  [user]  \r\n\tname=\"Grace Hopper\"\r\n\temail   =   g@h.io  \r\n");
  EXPECT_EQ(c.getUserName(), "Grace Hopper");
  EXPECT_EQ(c.getUserEmail(), "g@h.io");
}

TEST_F(ConfigTest, IgnoresCommentsAndOtherSections) {
  Config c = loadRepoConfig("; comment\n# another comment\n[core]\nname = not-user\n"
                            "[user]\nname = Real\nemail = r@r\n");
  EXPECT_EQ(c.getUserName(), "Real");
}

TEST_F(ConfigTest, MissingFieldsMeanNoIdentity) {
  EXPECT_FALSE(loadRepoConfig("").hasUserConfig());
  EXPECT_FALSE(loadRepoConfig("[user]\nname = Only Name\n").hasUserConfig());
  EXPECT_FALSE(loadRepoConfig("[user]\nemail = only@email\n").hasUserConfig());
}

TEST_F(ConfigTest, SaveThenLoadRoundTrips) {
  Config c(repo_);
  c.setUserName("Round Trip");
  c.setUserEmail("rt@example.com");
  c.save();

  Config loaded(repo_);
  loaded.load();
  EXPECT_EQ(loaded.getUserName(), "Round Trip");
  EXPECT_EQ(loaded.getUserEmail(), "rt@example.com");
}

// tmp/bugs/13
TEST_F(ConfigTest, ReadsGlobalConfigFromHome) {
  writeFile(home() / ".arkconfig", "[user]\nname = Global\nemail = global@example.com\n");
  Config c = loadRepoConfig("");
  EXPECT_EQ(c.getUserName(), "Global");
  EXPECT_EQ(c.getUserEmail(), "global@example.com");
}

// tmp/bugs/13
TEST_F(ConfigTest, ReadsXdgConfig) {
  writeFile(home() / ".config/ark/config", "[user]\nname = Xdg\nemail = xdg@example.com\n");
  Config c = loadRepoConfig("");
  EXPECT_EQ(c.getUserName(), "Xdg");
}

// tmp/bugs/13
TEST_F(ConfigTest, RepoConfigOverridesGlobal) {
  writeFile(home() / ".arkconfig", "[user]\nname = Global\nemail = global@example.com\n");
  Config c = loadRepoConfig("[user]\nname = Local\n");
  EXPECT_EQ(c.getUserName(), "Local");
  EXPECT_EQ(c.getUserEmail(), "global@example.com");
}
