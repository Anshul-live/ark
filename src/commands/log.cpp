#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <cat-file.h>
#include <ark.h>
#include <log.h>
#include <objects.h>
#include <ref.h>

void logBranch(const std::string& branchName) {
    Ref ref;
    std::string commitHash = ref.getBranchHash(branchName);
    
    int depth = -1;
    while (commitHash != NULL_HASH && (depth == -1 || depth--)) {
        std::string content = catFile(commitHash);
        std::vector<std::string> lines = split(content, '\n');
        std::vector<std::string> data;
        std::cout << "\033[1;33mcommit: " << commitHash << "\033[0m\n";
        for (auto line : lines) {
            std::vector<std::string> temp = split(line, ' ');
            data.insert(data.end(), temp.begin(), temp.end());
        }
        commitHash = data[3];
    }
}

int cmd_log(const std::vector<std::string> &args){
    Ref ref;
    std::string currentBranch = ref.getCurrentBranchName();
    
    if (currentBranch.empty()) {
        std::cout << "HEAD is detached\n";
        return 1;
    }
    
    logBranch(currentBranch);
    return 0;
}
