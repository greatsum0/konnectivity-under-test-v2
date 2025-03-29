#!/bin/bash
set -e

echo "🔄 [1/3] Resetting cluster..."
./reset.sh || echo "⚠️ Namespace not found. continue..."

echo "🔐 [2/3] Setting up auth..."
./auth.sh

echo "🚀 [3/3] Deploying to cluster..."
./deploy.sh

echo "✅ Done! Everything deployed successfully."
