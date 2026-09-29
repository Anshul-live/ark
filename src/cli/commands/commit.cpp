#include <ark/cli/commit_message_editor.h>
#include <ark/repo/config.h>
#include <iostream>
#include <ark/objects/objects.h>
#include <ark/repo/ref.h>

int cmd_commit(const std::vector<std::string> &args) {
  Ref ref;
  std::string commitHash = ref.getHeadCommit();

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

  std::string parentCommitHash = ref.getHeadCommit();
  Commit *commit = new Commit(message, parentCommitHash);
  commit->writeObjectToDisk();
  ref.setHeadToCommit(commit->hash);

  delete latestCommit;
  return 0;
}
