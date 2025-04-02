#!/bin/bash
set -e

# Absolute path
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Import utils and .env
source "$SCRIPT_DIR/utils.sh"
load_env

NAMESPACE=${NAMESPACE:-kut-app}

log "🧨 Deleting all resources in '$NAMESPACE' namespace..."
kubectl delete namespace "$NAMESPACE" --wait

log "🧼 Optionally cleaning up dangling PVs (if needed):"
if kubectl get pv | grep -q Released; then
  kubectl get pv | grep Released | awk '{print $1}' | xargs -r kubectl delete pv
  log "✅ Released PVs deleted."
else
  log "No released PVs found."
fi

log "✅ Cluster reset complete."
