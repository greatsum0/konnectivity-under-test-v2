#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

# Default namespace if not set in .env
NAMESPACE=${NAMESPACE:-kut-app}
SCENARIO_DIR=scenarios
CONFIGMAP_DIR=configmaps

echo "📂 Preparing directories..."
mkdir -p "$CONFIGMAP_DIR"

echo "🔄 Generating ConfigMaps from scenarios in '$SCENARIO_DIR/'..."

for file in "$SCENARIO_DIR"/*.yaml; do
  name=$(basename "$file" .yaml)
  configmap_name="kut-client-${name}-config"
  output_path="$CONFIGMAP_DIR/${configmap_name}.yaml"

  echo "🔧 Generating ConfigMap: $configmap_name → $output_path"

  kubectl create configmap "$configmap_name" \
    --from-file=scenario.yaml="$file" \
    --namespace="$NAMESPACE" \
    --dry-run=client -o yaml > "$output_path"
done

echo ""
echo "🧩 Generating kustomization.yaml in '$CONFIGMAP_DIR/'..."
generate_kustomization "$CONFIGMAP_DIR" "$NAMESPACE"

echo ""
echo "✅ All ConfigMaps generated in '$CONFIGMAP_DIR/'"
