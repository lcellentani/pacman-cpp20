#!/usr/bin/env bash
# .claude/hooks/guard-module-writes.sh

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | python -c "
import sys, json
d = json.load(sys.stdin)
path = d.get('tool_input', {}).get('file_path', '')
print(path)
")

NORMALIZED=$(echo "$FILE_PATH" | tr '\\' '/')

if [[ "$NORMALIZED" == *.ixx && "$NORMALIZED" != */src/* ]]; then
  echo '{
    "hookSpecificOutput": {
      "hookEventName": "PreToolUse",
      "permissionDecision": "deny",
      "permissionDecisionReason": ".ixx files must live in src/. Got: '"$FILE_PATH"'"
    }
  }'
  exit 0
fi

exit 0