#!/bin/bash
set -e

# Absolute path resolution
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

# Load utilities and environment
source "$SCRIPT_DIR/utils.sh"
load_env

NAMESPACE=${NAMESPACE:-kut-app}
JOB_DIR="$BASE_DIR/jobs"

log "🔁 Running scenario jobs one by one..."

for job_manifest in "$JOB_DIR"/job-*.yaml; do
  job_name=$(basename "$job_manifest" .yaml | sed 's/^job-//')

  log ""
  log "📤 Applying job: $job_name"
  kubectl delete job "kut-client-$job_name" -n "$NAMESPACE" --ignore-not-found --wait=true
  kubectl apply -f "$job_manifest" -n "$NAMESPACE"

  log "⏳ Waiting for pod of job 'kut-client-$job_name' to start..."

  # Wait for pod creation
  until pod_name=$(kubectl get pods -n "$NAMESPACE" -l job-name=kut-client-"$job_name" -o jsonpath='{.items[0].metadata.name}' 2>/dev/null); do
    sleep 1
  done

  log "🚀 Pod '$pod_name' created. Waiting for it to complete..."

  # Wait for pod completion
  while true; do
    phase=$(kubectl get pod "$pod_name" -n "$NAMESPACE" -o jsonpath='{.status.phase}')
    if [[ "$phase" == "Succeeded" ]]; then
      log "✅ Pod '$pod_name' completed successfully!"
      break
    elif [[ "$phase" == "Failed" ]]; then
      log "❌ Pod '$pod_name' failed!"
      break
    fi
    sleep 2
  done

  log "📄 Fetching logs for pod '$pod_name'..."
  kubectl logs "$pod_name" -n "$NAMESPACE"

  log "📦 Job 'kut-client-$job_name' finished at $(date +"%T")"
done

log ""
log "✅ All scenario jobs completed successfully!"
