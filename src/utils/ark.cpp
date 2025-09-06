#include <ark.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <cstdlib>
#include <unordered_map>

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string getEditor() {
    const char* editor = std::getenv("GIT_EDITOR");
    if (!editor) editor = std::getenv("VISUAL");
    if (!editor) editor = std::getenv("EDITOR");
    if (!editor) editor = "vi";
    return std::string(editor);
}

void openEditor(const std::string& editor, const std::string& file) {
    std::string cmd = editor + " " + file;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Editor exited with code " << ret << std::endl;
    }
}

std::string createTempFile() {
    std::string temp_file = std::filesystem::temp_directory_path() / "ark_commit_msg.txt";
    std::ofstream temp(temp_file); // create empty file
    return temp_file;
}

std::string readFileContent(const std::string& file) {
    std::ifstream in(file);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}


std::string getCommitMessageFromEditor() {
    std::string editor = getEditor();
    std::string tempFile = createTempFile();
    openEditor(editor, tempFile);
    std::string message = readFileContent(tempFile);
    std::filesystem::remove(tempFile); // cleanup
    return message;
}



std::string getMode(const std::filesystem::path& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        return 0; // error fallback
    }

    if (S_ISREG(st.st_mode)) {
        if (st.st_mode & S_IXUSR) return "100755"; // executable file
        else return "100644"; // regular file
    } else if (S_ISDIR(st.st_mode)) {
        return "040000"; // directory
    } else if (S_ISLNK(st.st_mode)) {
        return "120000"; // symlink
    }
    return "0"; // unknown type
}

std::unordered_map<std::string,std::string> loadConfig(){
  std::string repo_root = arkDir();
  std::string local_config_file = repo_root + "/.ark/config";
  std::cout<<local_config_file<<"\n";
}


