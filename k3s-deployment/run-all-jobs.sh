#!/bin/bash
set -e
source ./utils.sh
load_env

NAMESPACE=${NAMESPACE:-kut-app}
JOB_DIR=jobs

echo "🔁 Running scenario jobs one by one..."

for job_manifest in "$JOB_DIR"/job-*.yaml; do
  job_name=$(basename "$job_manifest" .yaml | sed 's/^job-//')

  echo ""
  echo "📤 Applying job: $job_name"

  kubectl delete job "kut-client-$job_name" -n "$NAMESPACE" --ignore-not-found
  kubectl apply -f "$job_manifest" -n "$NAMESPACE"

  echo "⏳ Waiting for job 'kut-client-$job_name' to complete..."
  kubectl wait --for=condition=complete --timeout=300s job/kut-client-"$job_name" -n "$NAMESPACE"
done

echo ""
echo "✅ All scenario jobs completed successfully!"
