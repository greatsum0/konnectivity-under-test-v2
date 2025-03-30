#!/bin/bash
set -e

echo "🧹 Cleaning previous manifests..."
rm -rf jobs/ configmaps/
mkdir -p jobs configmaps

echo "⚙️ Generating ConfigMaps from scenarios..."
./generate-configmaps.sh

echo "🏗️  Generating Job manifests..."
./generate-jobs.sh

echo "✅ All resources are ready. Run ./deploy.sh to apply them."
