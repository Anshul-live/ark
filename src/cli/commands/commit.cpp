#include <ark/cli/commit_message_editor.h>
#include <ark/repo/config.h>
#include <iostream>
#include <ark/objects/objects.h>
#include <ark/repo/ref_store.h>

int cmd_commit(const std::vector<std::string> &args) {
  RefStore ref;
  auto head = ref.resolveHead();
  std::string commitHash = head ? head->hex() : "";

  Commit *latestCommit = nullptr;
  if (!commitHash.empty()) {
    latestCommit = new Commit();
    latestCommit->loadFromDisk(commitHash);
  }

  bool hasChanges = true;
  if (latestCommit) {
    hasChanges = false;
  }

  if (!latestCommit || hasChanges) {
  }

  std::string message = CommitMessageEditor::prompt();
  if (message.empty()) {
    std::cerr << "commit aborted due to empty message";
    return 1;
  }

  auto parentHead = ref.resolveHead();
  std::string parentCommitHash = parentHead ? parentHead->hex() : "";
  Commit *commit = new Commit(message, parentCommitHash);
  commit->writeObjectToDisk();
  ref.moveHeadTo(ark::ObjectHash::parse(commit->hash).value());

  delete latestCommit;
  return 0;
}
