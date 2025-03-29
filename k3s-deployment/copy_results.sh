#!/bin/bash

# import utils functions
source ./utils.sh

# load the .env file
load_env

NAMESPACE=${NAMESPACE:-kut-app}

DEST_DIR="$HOME/Desktop/kut-data"
mkdir -p "$DEST_DIR/client"
mkdir -p "$DEST_DIR/server"

# Static name of the client pod
CLIENT_POD="pvc-viewer-client"

# Get the dynamic name of the server pod
SERVER_POD=$(kubectl get pods -n $NAMESPACE -l app=kut-server -o jsonpath="{.items[0].metadata.name}")

echo "Copying from server pod -> $SERVER_POD"

# Copy client data to local machine
kubectl cp "$NAMESPACE/$CLIENT_POD:/measurements/data" "$DEST_DIR/client"
echo "Client data copied to → $DEST_DIR/client"

# Copy server data to local machine
kubectl cp "$NAMESPACE/$SERVER_POD:/measurements/data" "$DEST_DIR/server"
echo "Server data copied to → $DEST_DIR/server"
