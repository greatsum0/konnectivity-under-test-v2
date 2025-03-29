#!/bin/bash

# utils.sh

load_env() {
  if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
  fi

  # Default fallback for namespace
  export NAMESPACE=${NAMESPACE:-kut-app}
}
