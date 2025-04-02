#!/bin/bash
set -e
source ./utils.sh

MEASUREMENT_DIR="measurement"
DEST_USER="username"
DEST_HOST="0.0.0.0"
DEST_PATH="/home/$DEST_USER/Desktop"
ZIP_FILE="/tmp/benchmark-results-$(date +%Y%m%d-%H%M%S).zip"
LOG_FILE="/var/log/kut-watcher.log"
ERROR_LOG="/var/log/kut-watcher.err"

log() {
  log "[$(date +'%F %T')] $1" | tee -a "$LOG_FILE"
}

handle_error() {
  log "[$(date +'%F %T')] ❌ ERROR: $1" | tee -a "$ERROR_LOG"
}

# 1. Check if CSV files exist
CSV_COUNT=$(find "$MEASUREMENT_DIR" -maxdepth 1 -name "*.csv" | wc -l)
if [[ "$CSV_COUNT" -eq 0 ]]; then
  handle_error "No CSV files found in $MEASUREMENT_DIR"
  exit 1
fi

# 2. Zip all measurement files
log "📦 Packaging $CSV_COUNT result file(s)..."
zip -j "$ZIP_FILE" "$MEASUREMENT_DIR"/*.csv > /dev/null

# 3. Transfer to user machine
log "📤 Sending results to $DEST_USER@$DEST_HOST:$DEST_PATH"
scp "$ZIP_FILE" "$DEST_USER@$DEST_HOST:$DEST_PATH" && log "✅ Results sent successfully."

# 4. Clean up temp zip (optional)
rm -f "$ZIP_FILE"
