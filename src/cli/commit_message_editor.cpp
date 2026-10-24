#include <ark/cli/commit_message_editor.h>
#include <ark/util/fs.h>
#include <ark/util/profiler.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

std::string CommitMessageEditor::resolveEditorCommand() {
    const char* editor = std::getenv("GIT_EDITOR");
    if (!editor) editor = std::getenv("VISUAL");
    if (!editor) editor = std::getenv("EDITOR");
    if (!editor) editor = "vi";
    return std::string(editor);
}

void CommitMessageEditor::runEditor(const std::string& editor, const std::string& file) {
    ARK_PROF_SCOPE("editor.run");
    std::string cmd = editor + " " + file;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Editor exited with code " << ret << std::endl;
    }
}

std::string CommitMessageEditor::createMessageFile() {
    std::string temp_file = std::filesystem::temp_directory_path() / "ark_commit_msg.txt";
    std::ofstream temp(temp_file);
    return temp_file;
}

std::string CommitMessageEditor::prompt() {
    std::string editor = resolveEditorCommand();
    std::string tempFile = createMessageFile();
    runEditor(editor, tempFile);
    std::string message = ark::readFileToString(tempFile);
    std::filesystem::remove(tempFile);
    return message;
}
