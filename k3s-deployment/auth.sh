#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

# check the mandatory variables
if [[ -z "$AUTH_USERNAME" || -z "$AUTH_TOKEN" || -z "$NAMESPACE" ]]; then
  echo "❌ AUTH_USERNAME or AUTH_TOKEN not set in .env file!"
  exit 1
fi

# create namespace if not exists
kubectl create namespace $NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

# create secret for docker registry
kubectl create secret docker-registry kut-regcred \
  --docker-username="$AUTH_USERNAME" \
  --docker-password="$AUTH_TOKEN" \
  -n $NAMESPACE \
  --dry-run=client -o yaml | kubectl apply -f -

echo "✅ Auth secret created/updated successfully."
