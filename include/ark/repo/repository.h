#ifndef REPOSITORY_HPP
#define REPOSITORY_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <string>


class Repository {
public:
  Repository();
  ~Repository();

  std::string root() const;
  std::string arkPath() const;
  std::string objectsDir() const;
  std::string indexPath() const;
  std::string configPath() const;
  std::string headPath() const;
  std::string refsDir() const;
  std::string logPath() const;

  bool isValid() const;

private:
  std::string findRoot() const;
  mutable std::string cachedRoot;
  mutable bool rootCached;
};

#endif
