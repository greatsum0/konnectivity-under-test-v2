#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

NAMESPACE=${NAMESPACE:-kut-app}

echo "🧨 Deleting all resources in '$NAMESPACE' namespace..."
kubectl delete namespace "$NAMESPACE" --wait

echo "🧼 Optionally cleaning up dangling PVs (if needed):"
kubectl get pv | grep Released || echo "No released PVs found."

echo "✅ Cluster reset complete."
