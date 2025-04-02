#!/bin/bash
set -e

# Absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Import utils and env
source "$SCRIPT_DIR/utils.sh"
load_env

# Config
NAMESPACE=${NAMESPACE:-kut-app}
IMAGE_NAME=${IMAGE_NAME}
SCENARIO_DIR="$BASE_DIR/scenarios"
JOB_DIR="$BASE_DIR/jobs"
TEMPLATE_FILE="$BASE_DIR/templates/job-template.yaml"

log "📂 Preparing job directory..."
mkdir -p "$JOB_DIR"

log "🛠️ Generating Job YAMLs from '$SCENARIO_DIR/'..."

for file in "$SCENARIO_DIR"/*.yaml; do
  name=$(basename "$file" .yaml)
  job_file="$JOB_DIR/job-${name}.yaml"

  log "📦 Creating job manifest: $job_file"

  sed -e "s/<scenario-name>/${name}/g" \
      -e "s/<scenario-file>/${name}/g" \
      -e "s|placeholder/temp-image|${IMAGE_NAME}|g" \
      "$TEMPLATE_FILE" > "$job_file"
done

log ""
generate_kustomization "$JOB_DIR" "$NAMESPACE"

log ""
log "✅ All job manifests created under '$JOB_DIR/'"
