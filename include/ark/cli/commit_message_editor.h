#ifndef ARK_HPP
#define ARK_HPP

#include <string>

class CommitMessageEditor {
public:
    static std::string prompt();

private:
    static std::string resolveEditorCommand();
    static void runEditor(const std::string& editor, const std::string& file);
    static std::string createMessageFile();
};

#endif
