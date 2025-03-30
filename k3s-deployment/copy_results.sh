#!/bin/bash

NAMESPACE="kut-app"
DEST_DIR="$HOME/Desktop/kut-data"
mkdir -p "$DEST_DIR/client"
mkdir -p "$DEST_DIR/server"

CLIENT_POD="pvc-viewer-client"

SERVER_POD=$(kubectl get pods -n $NAMESPACE -l app=kut-server -o jsonpath="{.items[0].metadata.name}")

echo "Kopyalanıyor: Server pod -> $SERVER_POD"

kubectl cp "$NAMESPACE/$CLIENT_POD:/measurements/data" "$DEST_DIR/client"
echo "Client verileri kopyalandı → $DEST_DIR/client"

kubectl cp "$NAMESPACE/$SERVER_POD:/measurements/data" "$DEST_DIR/server"
echo "Server verileri kopyalandı → $DEST_DIR/server"
