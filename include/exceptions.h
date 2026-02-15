#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

class ArkException : public std::runtime_error {
public:
    ArkException(const std::string& message) : std::runtime_error(message) {}
    virtual ~ArkException() = default;
    virtual std::string code() const { return "ARK_ERROR"; }
};

class RepositoryException : public ArkException {
public:
    RepositoryException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "REPO_ERROR"; }
};

class NotARepositoryException : public RepositoryException {
public:
    NotARepositoryException() : RepositoryException("Not inside an Ark repository") {}
    NotARepositoryException(const std::string& path) : RepositoryException("Not a repository: " + path) {}
    std::string code() const override { return "NOT_REPO"; }
};

class ObjectException : public ArkException {
public:
    ObjectException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "OBJECT_ERROR"; }
};

class ObjectNotFoundException : public ObjectException {
public:
    ObjectNotFoundException(const std::string& hash) 
        : ObjectException("Object not found: " + hash) {}
    std::string code() const override { return "OBJECT_NOT_FOUND"; }
};

class InvalidObjectException : public ObjectException {
public:
    InvalidObjectException(const std::string& message) 
        : ObjectException("Invalid object: " + message) {}
    std::string code() const override { return "INVALID_OBJECT"; }
};

class IndexException : public ArkException {
public:
    IndexException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "INDEX_ERROR"; }
};

class IndexCorruptedException : public IndexException {
public:
    IndexCorruptedException() : IndexException("Index file is corrupted") {}
    std::string code() const override { return "INDEX_CORRUPTED"; }
};

class BranchException : public ArkException {
public:
    BranchException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "BRANCH_ERROR"; }
};

class BranchNotFoundException : public BranchException {
public:
    BranchNotFoundException(const std::string& name)
        : BranchException("Branch not found: " + name) {}
    std::string code() const override { return "BRANCH_NOT_FOUND"; }
};

class BranchExistsException : public BranchException {
public:
    BranchExistsException(const std::string& name)
        : BranchException("Branch already exists: " + name) {}
    std::string code() const override { return "BRANCH_EXISTS"; }
};

class CommitException : public ArkException {
public:
    CommitException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "COMMIT_ERROR"; }
};

class NoCommitsException : public CommitException {
public:
    NoCommitsException() : CommitException("No commits found") {}
    std::string code() const override { return "NO_COMMITS"; }
};

class ConfigException : public ArkException {
public:
    ConfigException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "CONFIG_ERROR"; }
};

class ConfigNotFoundException : public ConfigException {
public:
    ConfigNotFoundException() : ConfigException("Configuration not found") {}
    std::string code() const override { return "CONFIG_NOT_FOUND"; }
};

class InvalidConfigException : public ConfigException {
public:
    InvalidConfigException(const std::string& message)
        : ConfigException("Invalid configuration: " + message) {}
    std::string code() const override { return "INVALID_CONFIG"; }
};

class FileSystemException : public ArkException {
public:
    FileSystemException(const std::string& message) : ArkException(message) {}
    std::string code() const override { return "FS_ERROR"; }
};

class FileNotFoundException : public FileSystemException {
public:
    FileNotFoundException(const std::string& path)
        : FileSystemException("File not found: " + path) {}
    std::string code() const override { return "FILE_NOT_FOUND"; }
};

class PermissionDeniedException : public FileSystemException {
public:
    PermissionDeniedException(const std::string& path)
        : FileSystemException("Permission denied: " + path) {}
    std::string code() const override { return "PERMISSION_DENIED"; }
};

#endif
