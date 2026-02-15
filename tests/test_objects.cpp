#include <gtest/gtest.h>
#include <objects.h>
#include <fstream>
#include <filesystem>

class ObjectsTest : public ::testing::Test {
protected:
    void SetUp() override {
        testFile = std::filesystem::temp_directory_path() / "ark_test_object.txt";
        std::ofstream out(testFile);
        out << "Test content for blob";
        out.close();
    }
    
    void TearDown() override {
        std::filesystem::remove(testFile);
    }
    
    std::filesystem::path testFile;
};

TEST_F(ObjectsTest, CreateBlobFromFile) {
    Blob blob(testFile.string());
    EXPECT_TRUE(blob.hash.length() > 0);
    EXPECT_EQ(blob.hash.length(), 64);
}

TEST_F(ObjectsTest, BlobHashIsSHA256) {
    Blob blob1(testFile.string());
    Blob blob2(testFile.string());
    EXPECT_EQ(blob1.hash, blob2.hash);
}

TEST_F(ObjectsTest, BlobContentFormat) {
    Blob blob(testFile.string());
    EXPECT_TRUE(blob.content.find("blob ") == 0);
    EXPECT_TRUE(blob.content.find('\0') != std::string::npos);
}

TEST_F(ObjectsTest, DifferentContentDifferentHash) {
    std::filesystem::path file1 = std::filesystem::temp_directory_path() / "test1.txt";
    std::filesystem::path file2 = std::filesystem::temp_directory_path() / "test2.txt";
    
    std::ofstream out1(file1);
    out1 << "Content A";
    out1.close();
    
    std::ofstream out2(file2);
    out2 << "Content B";
    out2.close();
    
    Blob blob1(file1.string());
    Blob blob2(file2.string());
    
    EXPECT_NE(blob1.hash, blob2.hash);
    
    std::filesystem::remove(file1);
    std::filesystem::remove(file2);
}
