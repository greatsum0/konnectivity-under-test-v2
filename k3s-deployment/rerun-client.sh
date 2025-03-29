#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

NAMESPACE=${NAMESPACE:-kut-app}

echo "🧨 Deleting previous kut-client job if it exists..."
kubectl delete job kut-client -n "$NAMESPACE" --ignore-not-found

echo "🚀 Reapplying kut-client job via Kustomize..."
kubectl apply -k .

echo "⏳ Waiting for pod to start..."
sleep 2
kubectl get pods -n "$NAMESPACE" -l job-name=kut-client

echo "✅ kut-client job restarted successfully!"
