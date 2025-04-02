#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

source "$SCRIPT_DIR/utils.sh"

NAMESPACE="kut-app"
DEST_DIR="$HOME/measurements"
mkdir -p "$DEST_DIR/client"
mkdir -p "$DEST_DIR/server"

# client pod name
CLIENT_POD="pvc-viewer-client"

# dynamic server pod name
SERVER_POD=$(kubectl get pods -n $NAMESPACE -l app=kut-server -o jsonpath="{.items[0].metadata.name}")

log "Copying: Server pod -> $SERVER_POD"

# Copy client data
kubectl cp "$NAMESPACE/$CLIENT_POD:/measurements/data" "$DEST_DIR/client"
log "Client data copied → $DEST_DIR/client"

# Copy server data
kubectl cp "$NAMESPACE/$SERVER_POD:/measurements/data" "$DEST_DIR/server"
log "Server data copied → $DEST_DIR/server"
