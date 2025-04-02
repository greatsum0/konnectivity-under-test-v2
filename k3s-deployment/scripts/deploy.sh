#!/bin/bash
set -e
source ./utils.sh
load_env

# Absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

NAMESPACE=${NAMESPACE:-kut-app}

log "🚀 [1/3] Creating namespace..."
kubectl apply -f "$BASE_DIR/namespace.yaml"

log "🔧 [2/3] Applying base manifests (config, PVC, deployments, services)..."
kubectl apply -k "$BASE_DIR/config/"
kubectl apply -k "$BASE_DIR/configmaps/"
kubectl apply -k "$BASE_DIR/deployments/"
kubectl apply -k "$BASE_DIR/pvc/"
kubectl apply -k "$BASE_DIR/services/"
kubectl apply -k "$BASE_DIR/tools/"

log "⏳ [3/3] Waiting for kut-server to become ready..."
kubectl wait --for=condition=available deployment/kut-server -n "$NAMESPACE" --timeout=60s

log "✅ Base deployment completed."
