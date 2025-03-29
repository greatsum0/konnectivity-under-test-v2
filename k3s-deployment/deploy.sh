#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

# Default namespace if not set in .env
NAMESPACE=${NAMESPACE:-kut-app}

echo ""
echo "🚀 [1/6] Creating namespace..."
kubectl apply -f namespace.yaml

echo ""
echo "🔧 [2/6] Applying manifests via Kustomize (config, pvc, deployments, services)..."
kubectl apply -k .

echo ""
echo "⏳ [3/6] Waiting for kut-server to become ready..."
kubectl wait --for=condition=available deployment/kut-server -n "$NAMESPACE" --timeout=60s

echo ""
echo "📡 [4/6] Running kut-client job..."

# Delete job if exists to prevent 'field is immutable' errors
kubectl delete job kut-client -n "$NAMESPACE" --ignore-not-found

# Apply again using Kustomize to get proper image override
kubectl apply -k .

echo ""
echo "📦 [5/6] Deploying PVC storage pod..."
kubectl apply -f tools/storage.yaml

echo ""
echo "✅ [6/6] All components deployed successfully!"
