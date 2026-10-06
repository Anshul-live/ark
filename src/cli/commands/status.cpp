#include <ark/cli/commands.h>
#include <ark/cli/commit_message_editor.h>
#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/objects/flat_tree.h>
#include <ark/repo/ignore_rules.h>
#include <ark/repo/index.h>
#include <ark/repo/ref_store.h>
#include <ark/repo/repository.h>
#include <ark/repo/status.h>
#include <ark/repo/working_tree.h>
#include <ark/storage/object_store.h>
#include <iostream>

int cmd_status(const Repository &repo, const std::vector<std::string> &args){
    ark::IgnoreRules ignore = ark::IgnoreRules::load(repo);
    ark::FlatTree worktree = ark::WorkingTree(repo, ignore).scan();

    Index idx(repo);
    idx.load();

    RefStore ref(repo);
    auto head = ref.resolveHead();
    std::string commitHash = head ? head->hex() : "";

    ark::FlatTree latest_committed_files;
    if (!commitHash.empty() && commitHash != ark::ObjectHash::zero().hex()) {
        ark::ObjectStore store(repo.objectsDir());
        ark::Commit commit =
            ark::CommitStore(store).read(ark::ObjectHash::unchecked(commitHash));
        latest_committed_files = ark::flattenTree(store, commit.tree);
    }

    ark::printStatus(ark::computeStatus(idx, latest_committed_files, worktree),
                     std::cout);
    return 0;
}
