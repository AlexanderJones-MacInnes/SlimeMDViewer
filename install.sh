#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_ROOT="${SLIME_INSTALL_ROOT:-$HOME/.local/share/slime}"
BIN_DIR="${SLIME_BIN_DIR:-$HOME/bin}"
CODEX_HOME="${CODEX_HOME:-$HOME/.codex}"
DOC_DIR="$INSTALL_ROOT/docs"
SKILL_DIR="$INSTALL_ROOT/skills/slime-doc-nav"
ACTIVE_SKILL_DIR="${SLIME_CODEX_SKILL_DIR:-$CODEX_HOME/skills/slime-doc-nav}"
BIN_NAME="slime_md_viewer"

cd "$PROJECT_ROOT"
make

mkdir -p "$INSTALL_ROOT" "$DOC_DIR" "$BIN_DIR" "$SKILL_DIR" "$ACTIVE_SKILL_DIR"

install -m 755 "build/$BIN_NAME" "$INSTALL_ROOT/$BIN_NAME"
rm -f "$DOC_DIR/MANUAL.md"
install -m 644 README.md DESIGN_SCOPE.md AGENTS.md "$DOC_DIR/"
install -m 644 skills/slime-doc-nav/SKILL.md "$SKILL_DIR/"
install -m 644 skills/slime-doc-nav/SKILL.md "$ACTIVE_SKILL_DIR/"

cat > "$BIN_DIR/slime" <<EOF
#!/usr/bin/env bash
exec "$INSTALL_ROOT/$BIN_NAME" "\$@"
EOF
chmod 755 "$BIN_DIR/slime"

cat <<EOF
Installed slime.

Binary:
  $INSTALL_ROOT/$BIN_NAME

Launcher:
  $BIN_DIR/slime

Docs:
  $DOC_DIR

Skill:
  $SKILL_DIR/SKILL.md

Active Codex skill:
  $ACTIVE_SKILL_DIR/SKILL.md
EOF
