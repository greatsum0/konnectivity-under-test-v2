#!/bin/bash
set -e

# Absolute path resolution
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

source "$SCRIPT_DIR/utils.sh"

WATCH_DIR="$BASE_DIR/scenarios"
HASH_FILE="/tmp/kut-last-change.hash"
TMP_HASH="/tmp/kut-temp.hash"
LOCK_FILE="/tmp/kut-pipeline.lock"
TRIGGER_SCRIPT="$SCRIPT_DIR/run.sh"
LOG_FILE="$BASE_DIR/kut-watcher.log"
COOLDOWN_SEC=5

# Snapshot helper
get_current_state() {
  find "$WATCH_DIR" -type f -name '*.yaml' -exec md5sum {} \; | sort
}

# Save initial state if missing
if [[ ! -f "$HASH_FILE" ]]; then
  get_current_state > "$HASH_FILE"
  log "📦 Initial scenario state snapshot saved."
fi

log "👀 Watching directory: $WATCH_DIR"

inotifywait -m -r -e create,modify --format "%w%f" "$WATCH_DIR" | while read FILE
do
  [[ "$FILE" != *.yaml ]] && continue
  log "🔍 Change detected: $FILE"

  get_current_state > "$TMP_HASH"

  if ! cmp -s "$HASH_FILE" "$TMP_HASH"; then
    log "✅ Real change detected. Preparing to trigger pipeline..."

    if [[ ! -f "$LOCK_FILE" ]]; then
      echo "$(date +%s)" > "$LOCK_FILE"
      cp "$TMP_HASH" "$HASH_FILE"

      {
        bash "$TRIGGER_SCRIPT" 2>&1
        log "✅ Pipeline completed successfully."
      } || {
        log "❌ Pipeline execution failed!"
      }

      log "🕓 Cooldown for $COOLDOWN_SEC seconds..."
      sleep "$COOLDOWN_SEC"
      rm -f "$LOCK_FILE"

    else
      log "⏳ Pipeline already running since $(cat $LOCK_FILE). Ignoring trigger."
    fi

  else
    log "ℹ️ No actual content change in scenarios. Skipping pipeline."
  fi
done
