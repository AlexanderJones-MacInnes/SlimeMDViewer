#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_NAME="slime"
VERSION="${1:-v0.1.0-alpha}"
ARCH="$(uname -m)"
DIST_ROOT="$PROJECT_ROOT/dist"
BUNDLE_DIR="$DIST_ROOT/${APP_NAME}-${VERSION}-linux-${ARCH}"
ARCHIVE_PATH="${BUNDLE_DIR}.tar.gz"

cd "$PROJECT_ROOT"
make

rm -rf "$BUNDLE_DIR" "$ARCHIVE_PATH"
mkdir -p "$BUNDLE_DIR/docs" "$BUNDLE_DIR/skills/slime-doc-nav"

install -m 755 "build/slime_md_viewer" "$BUNDLE_DIR/slime_md_viewer"
install -m 755 install.sh uninstall.sh "$BUNDLE_DIR/"
install -m 644 README.md CHANGELOG.md LICENSE DESIGN_SCOPE.md AGENTS.md "$BUNDLE_DIR/docs/"
install -m 644 skills/slime-doc-nav/SKILL.md "$BUNDLE_DIR/skills/slime-doc-nav/"

tar -czf "$ARCHIVE_PATH" -C "$DIST_ROOT" "$(basename "$BUNDLE_DIR")"

cat <<EOF
Release bundle created:
  $ARCHIVE_PATH
EOF
