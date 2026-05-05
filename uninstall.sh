#!/usr/bin/env bash
set -euo pipefail

INSTALL_ROOT="${SLIME_INSTALL_ROOT:-$HOME/.local/share/slime}"
BIN_DIR="${SLIME_BIN_DIR:-$HOME/bin}"
CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
ACTIVE_SKILL_DIR="${SLIME_CODEX_SKILL_DIR:-$CODEX_HOME/skills/slime-doc-nav}"

rm -f "$BIN_DIR/slime"
rm -rf "$INSTALL_ROOT"
rm -rf "$ACTIVE_SKILL_DIR"

cat <<EOF
Removed slime install artifacts.

Deleted:
  $BIN_DIR/slime
  $INSTALL_ROOT
  $ACTIVE_SKILL_DIR
EOF
