#include <gtest/gtest.h>
#include <repository.h>
#include <fstream>
#include <filesystem>

class RepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        testRepoDir = std::filesystem::temp_directory_path() / "ark_repo_test";
        std::filesystem::remove_all(testRepoDir);
        std::filesystem::create_directories(testRepoDir.string() + "/.ark");
        std::filesystem::create_directories(testRepoDir.string() + "/.ark/objects");
        std::filesystem::create_directories(testRepoDir.string() + "/.ark/refs/heads");
        
        std::ofstream out(testRepoDir.string() + "/.ark/HEAD");
        out << "ref: refs/heads/main";
        out.close();
        
        std::ofstream out2(testRepoDir.string() + "/.ark/refs/heads/main");
        out2 << "0000000000000000000000000000000000000000000000000000000000000000";
        out2.close();
        
        std::ofstream out3(testRepoDir.string() + "/.ark/index");
        out3.close();
        
        std::ofstream out4(testRepoDir.string() + "/.ark/config");
        out4.close();
        
        originalCwd = std::filesystem::current_path();
        std::filesystem::current_path(testRepoDir);
    }
    
    void TearDown() override {
        std::filesystem::current_path(originalCwd);
        std::filesystem::remove_all(testRepoDir);
    }
    
    std::filesystem::path testRepoDir;
    std::filesystem::path originalCwd;
};

TEST_F(RepositoryTest, IsValidRepository) {
    Repository repo;
    EXPECT_TRUE(repo.isValid());
}

TEST_F(RepositoryTest, RootPath) {
    Repository repo;
    std::string root = repo.root();
    EXPECT_TRUE(root.length() > 0);
}

TEST_F(RepositoryTest, ObjectsDir) {
    Repository repo;
    std::string objectsDir = repo.objectsDir();
    EXPECT_TRUE(objectsDir.find(".ark/objects") != std::string::npos);
}

TEST_F(RepositoryTest, IndexPath) {
    Repository repo;
    std::string indexPath = repo.indexPath();
    EXPECT_TRUE(indexPath.find(".ark/index") != std::string::npos);
}

TEST_F(RepositoryTest, ConfigPath) {
    Repository repo;
    std::string configPath = repo.configPath();
    EXPECT_TRUE(configPath.find(".ark/config") != std::string::npos);
}

TEST_F(RepositoryTest, HeadPath) {
    Repository repo;
    std::string headPath = repo.headPath();
    EXPECT_TRUE(headPath.find(".ark/HEAD") != std::string::npos);
}

TEST_F(RepositoryTest, RefsDir) {
    Repository repo;
    std::string refsDir = repo.refsDir();
    EXPECT_TRUE(refsDir.find(".ark/refs") != std::string::npos);
}

TEST_F(RepositoryTest, GetModeForFile) {
    std::ofstream out("testfile.txt");
    out << "test";
    out.close();
    
    Repository repo;
    std::string mode = repo.getMode("testfile.txt");
    EXPECT_TRUE(mode == "100644" || mode == "100755");
    
    std::filesystem::remove("testfile.txt");
}

TEST_F(RepositoryTest, GetModeForDirectory) {
    Repository repo;
    std::string mode = repo.getMode(".");
    EXPECT_EQ(mode, "040000");
}
