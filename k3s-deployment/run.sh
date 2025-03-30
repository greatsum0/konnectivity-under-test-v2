#!/bin/bash
set -e

echo "🔄 [1/4] Resetting cluster..."
./reset.sh || echo "⚠️ Namespace not found. continue..."

echo "🔐 [2/4] Setting up auth..."
./auth.sh

echo "🚀 [3/4] Deploying base components..."
./deploy.sh

echo "🧪 [4/4] Running scenario jobs one by one..."
./run-all-jobs.sh

echo ""
echo "✅ Done! All tests executed successfully."
