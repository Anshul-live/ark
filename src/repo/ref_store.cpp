#include <ark/cli/commit_message_editor.h>
#include <ark/core/exceptions.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ark/util/logger.h>
#include <ark/repo/ref_store.h>
#include <ark/repo/repository.h>

using ark::BranchName;
using ark::HeadTarget;
using ark::ObjectHash;
using ark::RefName;

namespace {

const std::string kSymbolicPrefix = "ref: ";
const std::string kHeadsPrefix = "refs/heads/";

std::string readHeadLine(const Repository &repo) {
  try {
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
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::readHead", e);
    throw;
  }
}

std::string readRefText(const Repository &repo, const std::string &refPath) {
  try {
    std::string fullPath = repo.root() + "/.ark/" + refPath;
    std::ifstream in(fullPath);
    if (!in) {
      return "";
    }
    std::string hash;
    in >> hash;
    in.close();
    return hash;
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::readRef", e);
    return "";
  }
}

std::optional<ObjectHash> hashFromText(const std::string &text) {
  if (text.empty())
    return std::nullopt;
  auto parsed = ObjectHash::parse(text);
  return parsed ? *parsed : ObjectHash::unchecked(text);
}

void writeRefText(const Repository &repo, const std::string &refPath,
                  const std::string &commitHash) {
  try {
    std::string fullPath = repo.root() + "/.ark/" + refPath;
    std::filesystem::create_directories(
        std::filesystem::path(fullPath).parent_path());
    std::ofstream out(fullPath);
    if (!out) {
      LOG_ERROR("Failed to write branch ref: " + fullPath);
      throw FileSystemException("Failed to write branch ref: " + refPath);
    }
    out << commitHash;
    out.close();
    LOG_DEBUG("Branch ref written: " + refPath + " -> " + commitHash);
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::writeRef", e);
    throw;
  }
}

} // namespace

RefStore::RefStore(const Repository &repo) : repo_(repo) {}

HeadTarget RefStore::readHead() const {
  std::string line = readHeadLine(repo_);
  if (line.rfind(kSymbolicPrefix, 0) == 0) {
    return RefName::unchecked(line.substr(kSymbolicPrefix.size()));
  }
  return ObjectHash::unchecked(line);
}

std::optional<ObjectHash> RefStore::resolveHead() const {
  try {
    HeadTarget head = readHead();
    if (const RefName *ref = std::get_if<RefName>(&head)) {
      return readRef(*ref);
    }
    return hashFromText(std::get<ObjectHash>(head).hex());
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::resolveHead", e);
    throw;
  }
}

std::optional<BranchName> RefStore::currentBranch() const {
  try {
    HeadTarget head = readHead();
    if (const RefName *ref = std::get_if<RefName>(&head)) {
      if (ref->str().rfind(kHeadsPrefix, 0) == 0) {
        std::string name = ref->str().substr(kHeadsPrefix.size());
        if (!name.empty())
          return BranchName::unchecked(name);
      }
    }
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::currentBranch", e);
  }
  return std::nullopt;
}

bool RefStore::isHeadDetached() const {
  try {
    return std::holds_alternative<ObjectHash>(readHead());
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::isHeadDetached", e);
    return false;
  }
}

void RefStore::moveHeadTo(const ObjectHash &commit) {
  LOG_INFO("Setting HEAD to commit: " + commit.hex());

  try {
    HeadTarget head = readHead();
    if (const RefName *ref = std::get_if<RefName>(&head)) {
      writeRefText(repo_, ref->str(), commit.hex());
      LOG_INFO("Updated branch reference to: " + commit.hex());
      return;
    }

    std::string headPath = repo_.headPath();
    std::ofstream out(headPath);
    if (!out) {
      LOG_ERROR("Failed to open HEAD file for writing: " + headPath);
      throw FileSystemException("Failed to update HEAD");
    }
    out << commit.hex();
    out.close();
    LOG_INFO("HEAD updated to: " + commit.hex());
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::moveHeadTo", e);
    throw;
  }
}

void RefStore::attachHeadTo(const BranchName &branch) {
  LOG_INFO("Switching to branch: " + branch.str());

  try {
    if (!branchExists(branch)) {
      LOG_ERROR("Branch does not exist: " + branch.str());
      throw BranchNotFoundException(branch.str());
    }

    std::string headPath = repo_.headPath();
    std::ofstream out(headPath);
    if (!out) {
      LOG_ERROR("Failed to open HEAD file for writing: " + headPath);
      throw FileSystemException("Failed to update HEAD");
    }
    out << kSymbolicPrefix << kHeadsPrefix << branch.str();
    out.close();
    LOG_INFO("Switched to branch: " + branch.str());
  } catch (const BranchException &e) {
    LOG_EXCEPTION("RefStore::attachHeadTo", e);
    throw;
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::attachHeadTo", e);
    throw;
  }
}

std::optional<ObjectHash> RefStore::readRef(const RefName &ref) const {
  return hashFromText(readRefText(repo_, ref.str()));
}

void RefStore::writeRef(const RefName &ref, const ObjectHash &commit) {
  LOG_INFO("Updating ref " + ref.str() + " to " + commit.hex());
  writeRefText(repo_, ref.str(), commit.hex());
}

std::optional<ObjectHash> RefStore::branchTarget(const BranchName &branch) const {
  return hashFromText(readRefText(repo_, kHeadsPrefix + branch.str()));
}

bool RefStore::branchExists(const BranchName &branch) const {
  return branchTarget(branch).has_value();
}

void RefStore::createBranch(const BranchName &branch, const ObjectHash &commit) {
  LOG_INFO("Creating branch: " + branch.str());

  try {
    if (branchExists(branch)) {
      LOG_ERROR("Branch already exists: " + branch.str());
      throw BranchExistsException(branch.str());
    }

    writeRefText(repo_, kHeadsPrefix + branch.str(), commit.hex());
    LOG_INFO("Branch created: " + branch.str());
  } catch (const BranchException &e) {
    LOG_EXCEPTION("RefStore::createBranch", e);
    throw;
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::createBranch", e);
    throw BranchException(std::string("Failed to create branch: ") + e.what());
  }
}

void RefStore::deleteBranch(const BranchName &branch) {
  LOG_INFO("Deleting branch: " + branch.str());

  try {
    std::string branchPath = repo_.refsDir() + "/heads/" + branch.str();
    if (!std::filesystem::exists(branchPath)) {
      LOG_ERROR("Branch not found: " + branch.str());
      throw BranchNotFoundException(branch.str());
    }
    std::filesystem::remove(branchPath);
    LOG_INFO("Branch deleted: " + branch.str());
  } catch (const BranchException &e) {
    LOG_EXCEPTION("RefStore::deleteBranch", e);
    throw;
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::deleteBranch", e);
    throw BranchException(std::string("Failed to delete branch: ") + e.what());
  }
}

std::vector<BranchName> RefStore::listBranches() const {
  std::vector<BranchName> branches;

  try {
    std::string refsPath = repo_.refsDir() + "/heads/";

    if (!std::filesystem::exists(refsPath)) {
      return branches;
    }

    for (const auto &entry : std::filesystem::directory_iterator(refsPath)) {
      if (entry.is_regular_file()) {
        branches.push_back(BranchName::unchecked(entry.path().filename().string()));
      }
    }
  } catch (const std::exception &e) {
    LOG_EXCEPTION("RefStore::listBranches", e);
  }

  return branches;
}
