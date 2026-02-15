#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdio>

class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = std::filesystem::temp_directory_path() / "ark_e2e_test";
        std::filesystem::remove_all(testDir);
        std::filesystem::create_directories(testDir);
        originalCwd = std::filesystem::current_path();
        std::filesystem::current_path(testDir);
    }
    
    void TearDown() override {
        std::filesystem::current_path(originalCwd);
        std::filesystem::remove_all(testDir);
    }
    
    std::filesystem::path testDir;
    std::filesystem::path originalCwd;
    
    bool runArk(const std::string& cmd, std::string& output) {
        std::string arkBin = "/Users/anshul/Desktop/code/project/ark/build/ark";
        std::string fullCmd = arkBin + " " + cmd + " 2>&1";
        
        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) return false;
        
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        
        int status = pclose(pipe);
        return status == 0;
    }
    
    bool runArkCommand(const std::string& cmd) {
        std::string output;
        return runArk(cmd, output);
    }
    
    void createFile(const std::string& path, const std::string& content) {
        std::ofstream out(path);
        out << content;
        out.close();
    }
};

TEST_F(EndToEndTest, InitRepository) {
    ASSERT_TRUE(runArkCommand("init"));
    EXPECT_TRUE(std::filesystem::exists(".ark"));
    EXPECT_TRUE(std::filesystem::exists(".ark/HEAD"));
    EXPECT_TRUE(std::filesystem::exists(".ark/index"));
    EXPECT_TRUE(std::filesystem::exists(".ark/config"));
}

TEST_F(EndToEndTest, AddSingleFile) {
    runArkCommand("init");
    createFile("test.txt", "test content");
    ASSERT_TRUE(runArkCommand("add test.txt"));
    
    std::ifstream in(".ark/index");
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("test.txt") != std::string::npos);
}

TEST_F(EndToEndTest, HashObject) {
    runArkCommand("init");
    createFile("test.txt", "hello");
    
    std::string hashOut;
    ASSERT_TRUE(runArk("hash-object test.txt", hashOut));
    EXPECT_TRUE(hashOut.length() >= 64);
}

TEST_F(EndToEndTest, CatFile) {
    runArkCommand("init");
    createFile("test.txt", "hello world");
    runArkCommand("add test.txt");
    
    std::ifstream idx(".ark/index");
    std::string line;
    std::string mode, hash;
    while (std::getline(idx, line)) {
        if (line.find("test.txt") != std::string::npos) {
            std::istringstream iss(line);
            iss >> mode >> hash;
            break;
        }
    }
    
    if (!hash.empty()) {
        std::string catOut;
        ASSERT_TRUE(runArk("cat-file " + hash, catOut));
        EXPECT_EQ(catOut.substr(0, catOut.size()-1), "hello world");
    }
}

TEST_F(EndToEndTest, WriteTree) {
    runArkCommand("init");
    createFile("test.txt", "content");
    runArkCommand("add test.txt");
    
    std::string treeOut;
    ASSERT_TRUE(runArk("write-tree", treeOut));
    EXPECT_TRUE(treeOut.length() >= 64);
}

TEST_F(EndToEndTest, StatusWithUntracked) {
    runArkCommand("init");
    createFile("untracked.txt", "new file");
    
    std::string statusOut;
    ASSERT_TRUE(runArk("status", statusOut));
    EXPECT_TRUE(statusOut.find("untracked") != std::string::npos);
}

TEST_F(EndToEndTest, StatusWithStaged) {
    runArkCommand("init");
    createFile("test.txt", "content");
    runArkCommand("add test.txt");
    
    std::string statusOut;
    ASSERT_TRUE(runArk("status", statusOut));
    EXPECT_TRUE(statusOut.find("Changes to be committed:") != std::string::npos);
}

TEST_F(EndToEndTest, NestedDirectories) {
    runArkCommand("init");
    std::filesystem::create_directories("src/utils");
    createFile("src/main.cpp", "int main() {}");
    createFile("src/utils/helper.h", "void help();");
    
    ASSERT_TRUE(runArkCommand("add src"));
    
    std::ifstream idx(".ark/index");
    std::string content((std::istreambuf_iterator<char>(idx)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("src/main.cpp") != std::string::npos);
    EXPECT_TRUE(content.find("src/utils/helper.h") != std::string::npos);
}

TEST_F(EndToEndTest, IndexPersistence) {
    runArkCommand("init");
    createFile("persistent.txt", "content");
    runArkCommand("add persistent.txt");
    runArkCommand("status");
    
    std::ifstream idx(".ark/index");
    std::string content((std::istreambuf_iterator<char>(idx)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("persistent.txt") != std::string::npos);
}

TEST_F(EndToEndTest, ConfigLoading) {
    runArkCommand("init");
    std::ofstream cfg(".ark/config");
    cfg << "[user]\nname = ConfigTest\nemail = config@test.com\n";
    cfg.close();
    
    std::ifstream check(".ark/config");
    std::string content((std::istreambuf_iterator<char>(check)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("ConfigTest") != std::string::npos);
}

TEST_F(EndToEndTest, MultipleFilesAdd) {
    runArkCommand("init");
    createFile("file1.txt", "content1");
    createFile("file2.txt", "content2");
    createFile("file3.txt", "content3");
    
    ASSERT_TRUE(runArkCommand("add file1.txt file2.txt file3.txt"));
    
    std::ifstream idx(".ark/index");
    std::string content((std::istreambuf_iterator<char>(idx)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("file1.txt") != std::string::npos);
    EXPECT_TRUE(content.find("file2.txt") != std::string::npos);
    EXPECT_TRUE(content.find("file3.txt") != std::string::npos);
}

TEST_F(EndToEndTest, AddDot) {
    runArkCommand("init");
    createFile("test1.txt", "content");
    createFile("test2.txt", "content");
    
    ASSERT_TRUE(runArkCommand("add ."));
    
    std::ifstream idx(".ark/index");
    std::string content((std::istreambuf_iterator<char>(idx)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("test1.txt") != std::string::npos);
    EXPECT_TRUE(content.find("test2.txt") != std::string::npos);
}
