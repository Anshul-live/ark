#!/bin/sh
# _extract_body.sh <decompressed_object_file>
# Reads the provided file in binary, finds first NUL byte, writes everything after it to stdout.
# Exits 0 on success, non-zero on failure.

if [ $# -ne 1 ]; then
  echo "Usage: $0 <decompressed_file>" >&2
  exit 2
fi

python3 - "$1" <<'PY'
import sys
path = sys.argv[1]
try:
    with open(path, "rb") as f:
        data = f.read()
except Exception as e:
    sys.stderr.write("READ_ERROR: %s\n" % e)
    sys.exit(3)

idx = data.find(b"\x00")
if idx == -1:
    sys.stderr.write("NO_NULL_IN_OBJECT\n")
    sys.exit(4)

# write the body (bytes after the NUL) to stdout (as text)
sys.stdout.buffer.write(data[idx+1:])
PY
