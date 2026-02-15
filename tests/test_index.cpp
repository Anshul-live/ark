#include <gtest/gtest.h>
#include <index.h>
#include <fstream>
#include <filesystem>

class IndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        testIndexFile = std::filesystem::temp_directory_path() / "ark_test_index";
        std::filesystem::remove(testIndexFile);
    }
    
    void TearDown() override {
        std::filesystem::remove(testIndexFile);
    }
    
    std::filesystem::path testIndexFile;
};

TEST_F(IndexTest, NewIndexIsEmpty) {
    Index idx;
    EXPECT_EQ(idx.size(), 0);
}

TEST_F(IndexTest, StageFile) {
    Index idx;
    idx.stage("test.txt", "abc123", "100644");
    EXPECT_EQ(idx.size(), 1);
    EXPECT_TRUE(idx.contains("test.txt"));
}

TEST_F(IndexTest, UnstageFile) {
    Index idx;
    idx.stage("test.txt", "abc123", "100644");
    idx.unstage("test.txt");
    EXPECT_EQ(idx.size(), 0);
}

TEST_F(IndexTest, GetEntry) {
    Index idx;
    idx.stage("test.txt", "abc123", "100644");
    const IndexEntry* entry = idx.get("test.txt");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->hash, "abc123");
    EXPECT_EQ(entry->mode, "100644");
}

TEST_F(IndexTest, GetEntryNotFound) {
    Index idx;
    const IndexEntry* entry = idx.get("nonexistent.txt");
    EXPECT_EQ(entry, nullptr);
}

TEST_F(IndexTest, ContainsFile) {
    Index idx;
    EXPECT_FALSE(idx.contains("test.txt"));
    idx.stage("test.txt", "abc123", "100644");
    EXPECT_TRUE(idx.contains("test.txt"));
}

TEST_F(IndexTest, ClearIndex) {
    Index idx;
    idx.stage("test1.txt", "hash1", "100644");
    idx.stage("test2.txt", "hash2", "100644");
    EXPECT_EQ(idx.size(), 2);
    idx.clear();
    EXPECT_EQ(idx.size(), 0);
}

TEST_F(IndexTest, MultipleEntries) {
    Index idx;
    idx.stage("file1.txt", "hash1", "100644");
    idx.stage("file2.txt", "hash2", "100755");
    idx.stage("dir/file3.txt", "hash3", "100644");
    
    EXPECT_EQ(idx.size(), 3);
    EXPECT_TRUE(idx.contains("file1.txt"));
    EXPECT_TRUE(idx.contains("file2.txt"));
    EXPECT_TRUE(idx.contains("dir/file3.txt"));
}
