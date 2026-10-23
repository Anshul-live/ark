#ifndef REF_STORE_HPP
#define REF_STORE_HPP

#include <ark/core/object_hash.h>
#include <ark/core/ref_name.h>
#include <ark/repo/repository.h>

#include <optional>
#include <vector>

class RefStore {
public:
    explicit RefStore(const Repository& repo);

    ark::HeadTarget readHead() const;
    std::optional<ark::ObjectHash> resolveHead() const;
    std::optional<ark::BranchName> currentBranch() const;
    bool isHeadDetached() const;

    void moveHeadTo(const ark::ObjectHash& commit);
    void attachHeadTo(const ark::BranchName& branch);

    std::optional<ark::ObjectHash> readRef(const ark::RefName& ref) const;
    void writeRef(const ark::RefName& ref, const ark::ObjectHash& commit);

    std::optional<ark::ObjectHash> branchTarget(const ark::BranchName& branch) const;
    bool branchExists(const ark::BranchName& branch) const;
    void createBranch(const ark::BranchName& branch, const ark::ObjectHash& commit);
    void deleteBranch(const ark::BranchName& branch);

    std::vector<ark::BranchName> listBranches() const;

private:
    const Repository& repo_;
};

#endif
