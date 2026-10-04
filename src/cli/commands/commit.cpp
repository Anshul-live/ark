#include <ark/cli/commit_message_editor.h>
#include <ark/objects/commit.h>
#include <ark/objects/commit_store.h>
#include <ark/objects/tree_builder.h>
#include <ark/repo/config.h>
#include <ark/repo/index.h>
#include <ark/repo/ref_store.h>
#include <ark/repo/repository.h>
#include <ark/storage/object_store.h>
#include <iostream>

int cmd_commit(const std::vector<std::string> &args) {
  RefStore ref;
  auto head = ref.resolveHead();
  std::string commitHash = head ? head->hex() : "";

  Repository repo;
  ark::ObjectStore store(repo.objectsDir());
  ark::CommitStore commits(store);

  if (!commitHash.empty() && commitHash != NULL_HASH) {
    commits.read(ark::ObjectHash::unchecked(commitHash));
  }

  std::string message = CommitMessageEditor::prompt();
  if (message.empty()) {
    std::cerr << "commit aborted due to empty message";
    return 1;
  }

  auto parentHead = ref.resolveHead();
  std::string parentCommitHash = parentHead ? parentHead->hex() : "";

  Config config;
  config.load();
  Index idx;
  idx.load();
  ark::Commit commit;
  commit.tree = ark::writeTreeFromIndex(idx, store);
  if (!config.hasUserConfig()) {
    std::cerr << "please provide the following values user.name and user.email "
                 "before committing.\n";
    exit(0);
  }
  std::cout << "passed checks" << std::endl;
  commit.parents.push_back(ark::ObjectHash::unchecked(parentCommitHash));
  commit.committer = ark::Signature::now(
      ark::Identity{config.getUserName(), config.getUserEmail()});
  commit.message = message;

  ark::ObjectHash written = commits.write(commit);
  ref.moveHeadTo(written);
  return 0;
}
