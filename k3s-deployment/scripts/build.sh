#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

source "$SCRIPT_DIR/utils.sh"

log "🧹 Cleaning previous manifests..."
rm -rf "$BASE_DIR/jobs/" "$BASE_DIR/configmaps/"
mkdir -p "$BASE_DIR/jobs" "$BASE_DIR/configmaps"

log "⚙️ Generating ConfigMaps from scenarios..."
bash "$SCRIPT_DIR/generate-configmaps.sh"

log "🏗️  Generating Job manifests..."
bash "$SCRIPT_DIR/generate-jobs.sh"

log "✅ All resources are ready. Run ./deploy.sh to apply them."
