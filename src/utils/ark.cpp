#include <ark.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

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
    std::ofstream temp(temp_file);
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
    std::filesystem::remove(tempFile);
    return message;
}
