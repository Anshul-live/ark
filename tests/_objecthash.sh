# Calculate blob hash exactly like ark
calc_blob_hash() {
  local file="$1"
  local size=$(wc -c <"$file" | tr -d ' ')
  # blob header + NUL + file content
  printf "blob %s\0" "$size"
  cat "$file"
}

blob_hash() {
  calc_blob_hash "$1" | shasum -a 256 | awk '{print $1}'
}

# Calculate tree hash using provided raw tree content
tree_hash() {
  local raw="$1"
  local size=${#raw}
  printf "tree %s\0%s" "$size" "$raw" | shasum -a 256 | awk '{print $1}'
}

# Calculate commit hash using provided raw commit content
commit_hash() {
  local raw="$1"
  local size=${#raw}
  printf "commit %s\0%s" "$size" "$raw" | shasum -a 256 | awk '{print $1}'
}
