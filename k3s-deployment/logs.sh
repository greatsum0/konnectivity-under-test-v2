#!/bin/bash

# import utils functions
source ./utils.sh

# load the .env file
load_env

NAMESPACE=${NAMESPACE:-kut-app}

echo "📄 Logs for client job (kut-client):"
kubectl logs job/kut-client -n "$NAMESPACE"