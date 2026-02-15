#include <gtest/gtest.h>
#include <ref.h>
#include <fstream>
#include <filesystem>

class RefTest : public ::testing::Test {
protected:
    void SetUp() override {
        testRepoDir = std::filesystem::temp_directory_path() / "ark_ref_test";
        std::filesystem::remove_all(testRepoDir);
        std::filesystem::create_directories(testRepoDir.string() + "/.ark/refs/heads");
        
        std::ofstream out(testRepoDir.string() + "/.ark/HEAD");
        out << "ref: refs/heads/main";
        out.close();
        
        std::ofstream out2(testRepoDir.string() + "/.ark/refs/heads/main");
        out2 << "abc123def456789012345678901234567890123456789012345678901234";
        out2.close();
        
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

TEST_F(RefTest, GetHeadCommit) {
    Ref ref;
    std::string commit = ref.getHeadCommit();
    EXPECT_EQ(commit, "abc123def456789012345678901234567890123456789012345678901234");
}

TEST_F(RefTest, GetCurrentBranchName) {
    Ref ref;
    std::string branch = ref.getCurrentBranchName();
    EXPECT_EQ(branch, "main");
}

TEST_F(RefTest, IsDetached) {
    Ref ref;
    EXPECT_FALSE(ref.isDetached());
}

TEST_F(RefTest, BranchExists) {
    Ref ref;
    EXPECT_TRUE(ref.branchExists("main"));
    EXPECT_FALSE(ref.branchExists("nonexistent"));
}

TEST_F(RefTest, ListBranches) {
    Ref ref;
    auto branches = ref.listBranches();
    EXPECT_EQ(branches.size(), 1);
    EXPECT_EQ(branches[0], "main");
}
