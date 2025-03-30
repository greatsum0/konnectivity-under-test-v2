#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

# Default namespace if not set in .env
NAMESPACE=${NAMESPACE:-kut-app}
IMAGE_NAME=${IMAGE_NAME}
SCENARIO_DIR=scenarios
JOB_DIR=jobs

echo "📂 Preparing job directory..."
mkdir -p "$JOB_DIR"

echo "🛠️ Generating Job YAMLs from '$SCENARIO_DIR/'..."

for file in "$SCENARIO_DIR"/*.yaml; do
  name=$(basename "$file" .yaml)
  job_file="$JOB_DIR/job-${name}.yaml"

  echo "📦 Creating job manifest: $job_file"

  sed -e "s/<scenario-name>/${name}/g" \
      -e "s/<scenario-file>/${name}/g" \
      -e "s|placeholder/temp-image|${IMAGE_NAME}|g" \
      job-template.yaml > "$job_file"
done

echo ""
echo "🧩 Generating kustomization.yaml in '$JOB_DIR/'..."
generate_kustomization "$JOB_DIR" "$NAMESPACE"

echo ""
echo "✅ All job manifests created under '$JOB_DIR/'"
