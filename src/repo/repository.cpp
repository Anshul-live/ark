#include <algorithm>
#include <ark/cli/commit_message_editor.h>
#include <cmath>
#include <ark/repo/config.h>
#include <ark/core/exceptions.h>
#include <fstream>
#include <ark/cli/commands.h>
#include <iostream>
#include <ark/util/fs.h>
#include <ark/util/logger.h>
#include <ark/util/strings.h>
#include <ark/repo/repository.h>
#include <sstream>
#include <sys/stat.h>

Repository::Repository() : rootCached(false) {}

Repository::~Repository() {}

std::string Repository::root() const {
  if (!rootCached) {
    try {
      cachedRoot = findRoot();
      rootCached = true;
    } catch (const std::exception &e) {
      LOG_EXCEPTION("Repository::root", e);
      throw NotARepositoryException();
    }
  }
  return cachedRoot;
}

std::string Repository::arkPath() const { return root() + "/.ark"; }

std::string Repository::findRoot() const {
  std::filesystem::path current = std::filesystem::current_path();

  while (true) {
    if (std::filesystem::exists(current / ".ark") &&
        std::filesystem::is_directory(current / ".ark")) {
      return current.string();
    }

    if (current.has_parent_path() && current.string() != "/") {
      current = current.parent_path();
    } else {
      LOG_ERROR("Not inside an Ark repository");
      throw NotARepositoryException(current.string());
    }
  }
}

bool Repository::isValid() const {
  try {
    std::string r = root();
    return std::filesystem::exists(r + "/.ark");
  } catch (const std::exception &e) {
    LOG_EXCEPTION("Repository::isValid", e);
    return false;
  }
}

std::string Repository::objectsDir() const {
  try {
    return root() + "/.ark/objects";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::objectsDir", e);
    throw;
  }
}

std::string Repository::indexPath() const {
  try {
    return root() + "/.ark/index";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::indexPath", e);
    throw;
  }
}

std::string Repository::configPath() const {
  try {
    return root() + "/.ark/config";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::configPath", e);
    throw;
  }
}

std::string Repository::headPath() const {
  try {
    return root() + "/.ark/HEAD";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::headPath", e);
    throw;
  }
}

std::string Repository::refsDir() const {
  try {
    return root() + "/.ark/refs";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::refsDir", e);
    throw;
  }
}

std::string Repository::logPath() const {
  try {
    return root() + "/.ark/ark.log";
  } catch (const NotARepositoryException &e) {
    LOG_EXCEPTION("Repository::logPath", e);
    throw;
  }
}
