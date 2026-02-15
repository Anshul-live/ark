#ifndef REF_HPP
#define REF_HPP

#include <string>
#include <vector>

class Ref {
public:
    Ref();

    std::string getHeadCommit() const;
    std::string getHeadBranch() const;
    bool isDetached() const;
    
    void setHeadToCommit(const std::string& commitHash);
    void setHeadToBranch(const std::string& branchName);
    void updateBranch(const std::string& branchName, const std::string& commitHash);
    void updateRef(const std::string& refPath, const std::string& commitHash);
    
    std::string getBranchHash(const std::string& branchName) const;
    bool branchExists(const std::string& branchName) const;
    void createBranch(const std::string& branchName, const std::string& commitHash);
    void deleteBranch(const std::string& branchName);
    
    std::vector<std::string> listBranches() const;
    std::string getCurrentBranchName() const;

private:
    std::string readHeadContent() const;
    std::string readBranchRef(const std::string& branchName) const;
    void writeBranchRef(const std::string& branchName, const std::string& commitHash);
};

#endif
