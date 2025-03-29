#!/bin/bash
set -e

# import utils functions
source ./utils.sh

# load the .env file
load_env

echo "🔎 Cluster Status for namespace '$NAMESPACE'"

echo -e "\n🧱 Nodes:"
kubectl get nodes

echo -e "\n📦 Deployments:"
kubectl get deployments -n "$NAMESPACE"

echo -e "\n📋 Jobs:"
kubectl get jobs -n "$NAMESPACE"

echo -e "\n🧠 Pods:"
kubectl get pods -n "$NAMESPACE"

echo -e "\n📂 PVCs (Persistent Volume Claims):"
kubectl get pvc -n "$NAMESPACE"

echo -e "\n💾 PVs (Persistent Volumes):"
kubectl get pv

echo -e "\n🔒 Secrets:"
kubectl get secrets -n "$NAMESPACE"

echo -e "\n🔧 ConfigMaps:"
kubectl get configmap -n "$NAMESPACE"

echo -e "\n🌐 Services:"
kubectl get svc -n "$NAMESPACE"

echo -e "\n🛰️ Ingresses:"
kubectl get ingress -n "$NAMESPACE" || echo "⚠️ Ingress not installed"

echo -e "\n✅ Events (last 10):"
kubectl get events -n "$NAMESPACE" --sort-by='.lastTimestamp' | tail -n 10
