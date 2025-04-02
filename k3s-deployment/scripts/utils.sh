#!/bin/bash

# utils.sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"
LOG_FILE="$BASE_DIR/kut-watcher.log"

load_env() {
  if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
  fi

  # Default fallback for namespace
  export NAMESPACE=${NAMESPACE:-kut-app}
}

generate_kustomization() {
  local target_dir="$1"
  local namespace="$2"
  local header_comment="${3:-}"

  local kustom_file="$target_dir/kustomization.yaml"
  local image_name="${IMAGE_NAME:-oguznagak/kut-tester}"
  local image_tag="${IMAGE_TAG:-latest}"

  log "🧩 Generating kustomization.yaml in $target_dir"

  {
    echo "apiVersion: kustomize.config.k8s.io/v1beta1"
    echo "kind: Kustomization"
    echo "namespace: $namespace"
    echo ""
    echo "resources:"
    for file in "$target_dir"/*.yaml; do
      fname=$(basename "$file")
      [[ "$fname" == "kustomization.yaml" ]] && continue
      echo "  - $fname"
    done
    echo ""
    echo "images:"
    echo "  - name: placeholder/temp-image"
    echo "    newName: $image_name"
    echo "    newTag: $image_tag"
  } > "$kustom_file"

  log "✅ kustomization.yaml created at $kustom_file"
}

log() {
  local SCRIPT_NAME
  SCRIPT_NAME=$(basename "$0")
  local TIMESTAMP
  TIMESTAMP=$(date +'%F %T')
  local MESSAGE="[$TIMESTAMP] [$SCRIPT_NAME] $*"

  echo "$MESSAGE" | tee -a "$LOG_FILE"
}
