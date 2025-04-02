#!/bin/bash
set -e

# Absolute path setup
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Load utils
source "$SCRIPT_DIR/utils.sh"

log "🔄 [1/4] Resetting cluster..."
if ! bash "$SCRIPT_DIR/reset.sh"; then
  log "⚠️ Namespace not found or already deleted. Continuing anyway..."
fi

log "🧱 [2/4] Building manifests..."
bash "$SCRIPT_DIR/build.sh"

log "🔐 [3/4] Setting up Docker auth..."
bash "$SCRIPT_DIR/docker-auth.sh"

log "🚀 [4/4] Deploying base components..."
bash "$SCRIPT_DIR/deploy.sh"

log "🧪 [5/5] Running scenario jobs one by one..."
bash "$SCRIPT_DIR/run-all-jobs.sh"

log ""
log "✅ Done! All tests executed successfully."
