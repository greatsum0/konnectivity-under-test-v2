#!/bin/bash
set -e

# Get absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Source utils and env
source "$SCRIPT_DIR/utils.sh"
load_env

# Check required environment variables
if [[ -z "$AUTH_USERNAME" || -z "$AUTH_TOKEN" || -z "$NAMESPACE" ]]; then
  log "❌ AUTH_USERNAME or AUTH_TOKEN or NAMESPACE not set in .env file!"
  exit 1
fi

# Create namespace if it doesn't exist
kubectl create namespace "$NAMESPACE" --dry-run=client -o yaml | kubectl apply -f -

# Create or update docker registry secret
kubectl create secret docker-registry kut-regcred \
  --docker-username="$AUTH_USERNAME" \
  --docker-password="$AUTH_TOKEN" \
  -n "$NAMESPACE" \
  --dry-run=client -o yaml | kubectl apply -f -

log "✅ Auth secret created/updated successfully."
