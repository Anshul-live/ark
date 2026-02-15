#include <ref.h>
#include <ark.h>
#include <repository.h>
#include <logger.h>
#include <exceptions.h>
#include <fstream>
#include <iostream>
#include <filesystem>

Ref::Ref() {}

std::string Ref::readHeadContent() const {
    try {
        Repository repo;
        std::string headPath = repo.headPath();
        std::ifstream in(headPath);
        if (!in) {
            LOG_ERROR("Failed to open HEAD file: " + headPath);
            throw FileSystemException("Failed to open HEAD file");
        }
        std::string line;
        std::getline(in, line);
        in.close();
        return line;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::readHeadContent", e);
        throw;
    }
}

std::string Ref::getHeadCommit() const {
    try {
        std::string line = readHeadContent();
        if (line.rfind("ref: ", 0) == 0) {
            std::string branchPath = line.substr(5);
            return readBranchRef(branchPath);
        }
        return line;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::getHeadCommit", e);
        throw;
    }
}

std::string Ref::getHeadBranch() const {
    try {
        std::string line = readHeadContent();
        if (line.rfind("ref: ", 0) == 0) {
            return line.substr(5);
        }
        return "";
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::getHeadBranch", e);
        throw;
    }
}

bool Ref::isDetached() const {
    try {
        std::string line = readHeadContent();
        return line.rfind("ref: ", 0) != 0;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::isDetached", e);
        return false;
    }
}

void Ref::setHeadToCommit(const std::string& commitHash) {
    LOG_INFO("Setting HEAD to commit: " + commitHash);
    
    try {
        std::string line = readHeadContent();
        if (line.rfind("ref: ", 0) == 0) {
            std::string branchPath = line.substr(5);
            writeBranchRef(branchPath, commitHash);
            LOG_INFO("Updated branch reference to: " + commitHash);
            return;
        }
        
        Repository repo;
        std::string headPath = repo.headPath();
        std::ofstream out(headPath);
        if (!out) {
            LOG_ERROR("Failed to open HEAD file for writing: " + headPath);
            throw FileSystemException("Failed to update HEAD");
        }
        out << commitHash;
        out.close();
        LOG_INFO("HEAD updated to: " + commitHash);
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::setHeadToCommit", e);
        throw;
    }
}

void Ref::setHeadToBranch(const std::string& branchName) {
    LOG_INFO("Switching to branch: " + branchName);
    
    try {
        if (!branchExists(branchName)) {
            LOG_ERROR("Branch does not exist: " + branchName);
            throw BranchNotFoundException(branchName);
        }
        
        Repository repo;
        std::string headPath = repo.headPath();
        std::ofstream out(headPath);
        if (!out) {
            LOG_ERROR("Failed to open HEAD file for writing: " + headPath);
            throw FileSystemException("Failed to update HEAD");
        }
        out << "ref: refs/heads/" << branchName;
        out.close();
        LOG_INFO("Switched to branch: " + branchName);
    } catch (const BranchException& e) {
        LOG_EXCEPTION("Ref::setHeadToBranch", e);
        throw;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::setHeadToBranch", e);
        throw;
    }
}

void Ref::updateBranch(const std::string& branchName, const std::string& commitHash) {
    LOG_INFO("Updating branch " + branchName + " to " + commitHash);
    writeBranchRef("refs/heads/" + branchName, commitHash);
}

std::string Ref::readBranchRef(const std::string& branchPath) const {
    try {
        Repository repo;
        std::string fullPath = repo.root() + "/.ark/" + branchPath;
        std::ifstream in(fullPath);
        if (!in) {
            return "";
        }
        std::string hash;
        in >> hash;
        in.close();
        return hash;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::readBranchRef", e);
        return "";
    }
}

void Ref::writeBranchRef(const std::string& branchPath, const std::string& commitHash) {
    try {
        Repository repo;
        std::string fullPath = repo.root() + "/.ark/" + branchPath;
        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        std::ofstream out(fullPath);
        if (!out) {
            LOG_ERROR("Failed to write branch ref: " + fullPath);
            throw FileSystemException("Failed to write branch ref: " + branchPath);
        }
        out << commitHash;
        out.close();
        LOG_DEBUG("Branch ref written: " + branchPath + " -> " + commitHash);
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::writeBranchRef", e);
        throw;
    }
}

std::string Ref::getBranchHash(const std::string& branchName) const {
    return readBranchRef("refs/heads/" + branchName);
}

bool Ref::branchExists(const std::string& branchName) const {
    std::string hash = getBranchHash(branchName);
    return !hash.empty();
}

void Ref::createBranch(const std::string& branchName, const std::string& commitHash) {
    LOG_INFO("Creating branch: " + branchName);
    
    try {
        if (branchExists(branchName)) {
            LOG_ERROR("Branch already exists: " + branchName);
            throw BranchExistsException(branchName);
        }
        
        writeBranchRef("refs/heads/" + branchName, commitHash);
        LOG_INFO("Branch created: " + branchName);
    } catch (const BranchException& e) {
        LOG_EXCEPTION("Ref::createBranch", e);
        throw;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::createBranch", e);
        throw BranchException(std::string("Failed to create branch: ") + e.what());
    }
}

void Ref::deleteBranch(const std::string& branchName) {
    LOG_INFO("Deleting branch: " + branchName);
    
    try {
        Repository repo;
        std::string branchPath = repo.refsDir() + "/heads/" + branchName;
        if (!std::filesystem::exists(branchPath)) {
            LOG_ERROR("Branch not found: " + branchName);
            throw BranchNotFoundException(branchName);
        }
        std::filesystem::remove(branchPath);
        LOG_INFO("Branch deleted: " + branchName);
    } catch (const BranchException& e) {
        LOG_EXCEPTION("Ref::deleteBranch", e);
        throw;
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::deleteBranch", e);
        throw BranchException(std::string("Failed to delete branch: ") + e.what());
    }
}

std::vector<std::string> Ref::listBranches() const {
    std::vector<std::string> branches;
    
    try {
        Repository repo;
        std::string refsPath = repo.refsDir() + "/heads/";
        
        if (!std::filesystem::exists(refsPath)) {
            return branches;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(refsPath)) {
            if (entry.is_regular_file()) {
                branches.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::listBranches", e);
    }
    
    return branches;
}

std::string Ref::getCurrentBranchName() const {
    try {
        std::string line = readHeadContent();
        if (line.rfind("ref: ", 0) == 0) {
            std::string branchPath = line.substr(5);
            if (branchPath.rfind("refs/heads/", 0) == 0) {
                return branchPath.substr(11);
            }
        }
    } catch (const std::exception& e) {
        LOG_EXCEPTION("Ref::getCurrentBranchName", e);
    }
    return "";
}
