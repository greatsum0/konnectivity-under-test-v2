#!/bin/bash
set -e
source ./utils.sh
load_env

NAMESPACE=${NAMESPACE:-kut-app}

echo "🚀 [1/3] Creating namespace..."
kubectl apply -f namespace.yaml

echo "🔧 [2/3] Applying base manifests (config, PVC, deployments, services)..."
kubectl apply -k config/
kubectl apply -k configmaps/
kubectl apply -k deployments/
kubectl apply -k pvc/
kubectl apply -k services/
kubectl apply -k tools/

echo "⏳ [3/3] Waiting for kut-server to become ready..."
kubectl wait --for=condition=available deployment/kut-server -n "$NAMESPACE" --timeout=60s

echo "✅ Base deployment completed."