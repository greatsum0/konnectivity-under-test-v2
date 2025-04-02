#!/bin/bash
set -e

# Get absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Import utils and load env
source "$SCRIPT_DIR/utils.sh"
load_env

# Variables
NAMESPACE=${NAMESPACE:-kut-app}
SCENARIO_DIR="$BASE_DIR/scenarios"
CONFIGMAP_DIR="$BASE_DIR/configmaps"

log "📂 Preparing directories..."
mkdir -p "$CONFIGMAP_DIR"

log "🔄 Generating ConfigMaps from scenarios in '$SCENARIO_DIR/'..."

for file in "$SCENARIO_DIR"/*.yaml; do
  name=$(basename "$file" .yaml)
  configmap_name="kut-client-${name}-config"
  output_path="$CONFIGMAP_DIR/${configmap_name}.yaml"

  log "🔧 Generating ConfigMap: $configmap_name → $output_path"

  kubectl create configmap "$configmap_name" \
    --from-file=scenario.yaml="$file" \
    --namespace="$NAMESPACE" \
    --dry-run=client -o yaml > "$output_path"
done

log ""
generate_kustomization "$CONFIGMAP_DIR" "$NAMESPACE"

log ""
log "✅ All ConfigMaps generated in '$CONFIGMAP_DIR/'"
