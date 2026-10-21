#!/usr/bin/env bash
# Download the gta_reversed.asi built by GitHub Actions for a branch of the fork.
#
# Usage: tools/mac/fetch-ci-asi.sh <branch> [Release|Debug]   (default: Release)
# Prints the path of the downloaded .asi.
set -euo pipefail

REPO_SLUG="nic0der-im/gta-reversed"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <branch> [Release|Debug]" >&2
    exit 2
fi
BRANCH="$1"
CONF="${2:-Release}"

RUN="$(gh run list -R "$REPO_SLUG" --workflow Build --branch "$BRANCH" --status success --limit 1 --json databaseId -q '.[0].databaseId')"
if [ -z "$RUN" ]; then
    echo "No successful Build run for $BRANCH" >&2
    exit 1
fi

DEST="$ROOT/.local/asi/${BRANCH//\//-}-$CONF"
rm -rf "$DEST"
gh run download "$RUN" -R "$REPO_SLUG" -n "gta_reversed-$CONF-Unity" -D "$DEST"
find "$DEST" -name '*.asi' -print
