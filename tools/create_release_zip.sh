#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_NAME="infisolar_pi18-esphome.zip"
OUTPUT_PATH="${REPO_ROOT}/${OUTPUT_NAME}"

if [[ $# -gt 1 ]]; then
  echo "Usage: $0 [output-zip]" >&2
  exit 1
fi

if [[ $# -eq 1 ]]; then
  OUTPUT_PATH="$1"
fi

if [[ -e "${OUTPUT_PATH}" ]]; then
  rm -f "${OUTPUT_PATH}"
fi

pushd "${REPO_ROOT}" >/dev/null

git archive --format=zip --output="${OUTPUT_PATH}" HEAD

popd >/dev/null

echo "Release archive generated at: ${OUTPUT_PATH}"
