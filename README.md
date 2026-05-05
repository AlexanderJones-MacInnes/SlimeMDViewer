# SlimeMDViewer

Small local markdown viewer for Linux.

What it does:
- opens a markdown file
- renders it in a native Qt window
- uses local parsers instead of inventing one
- watches the file and reloads when it changes
- supports lightweight live control commands
- supports per-instance targeting with `--pid` when multiple windows are open

Current parser order:
1. `pandoc`
2. `python3 -m markdown`

This is intentionally not a note suite and not a browser tab. It is just a local desktop viewer with a decent-looking document surface.

## Dependencies

Build dependencies:
- `g++`
- `make`
- `pkg-config`
- `qt6-base-dev` or `qtbase5-dev`
- matching Qt network module (included in the base dev packages on normal Debian/Ubuntu installs)

Runtime parser dependencies:
- `pandoc`
- or `python3` plus the `markdown` module

Example Ubuntu/Debian packages:

```bash
sudo apt install build-essential make pkg-config qt6-base-dev pandoc python3-markdown
```

If you prefer Qt 5 instead, `qtbase5-dev` should also work.

## Build

```bash
make
```

Create a release bundle:

```bash
make package
```

That writes a versioned tarball under `dist/`.

## Install

```bash
make install
```

This installs:

- the binary under `~/.local/share/slime/`
- a `slime` launcher into `~/bin`
- project docs alongside the installed binary
- the shipped copy of the skill under `~/.local/share/slime/skills/slime-doc-nav/`
- the active Codex skill under `~/.codex/skills/slime-doc-nav/` by default

Remove it with:

```bash
make uninstall
```

## Run

After `make install`, launch with:

```bash
slime
```

Open a specific markdown file:

```bash
slime /path/to/file.md
```

Open on a specific monitor:

```bash
slime /path/to/file.md --monitor 1
```

Jump an already-running viewer to an anchor:

```bash
slime --scroll first-pass-file-map
```

Jump a specific running viewer by PID:

```bash
slime --pid 12345 --scroll first-pass-file-map
```

Show information for the most recent running instance:

```bash
slime --info
```

Dump the currently visible text from a running instance:

```bash
slime --dump-visible
```

List known running instances:

```bash
slime --list-instances
```

For local development without installing:

```bash
./build/slime_md_viewer /path/to/file.md
```

## Menus And Shortcuts

Current top-level menus:

- `File`
- `Settings`

Current shortcuts:

- `Ctrl+O` opens a file
- `Ctrl+R` reloads the current file
- `Ctrl+Q` quits the window

## Human Use

Good human workflows:

- open a working document and keep it live while editing
- use file auto-reload instead of reopening manually
- use separate windows when comparing a few docs
- use inline HTML lightly if markdown alone is not enough

`slime` currently handles simple inline HTML well enough for practical things like:

- colored text
- highlighted spans
- basic inline emphasis combinations

## Agent Skill

This project ships a matching Codex skill at [skills/slime-doc-nav/SKILL.md](./skills/slime-doc-nav/SKILL.md).

That skill is the agent-facing operational playbook for:

- opening the right markdown file
- targeting the right live instance
- scrolling to anchors
- querying visible text for “look at this” style prompts

The installer also copies that skill into your Codex skills directory by default.
It also keeps a shipped copy alongside the installed tool.

## Notes

- Relative images and links are resolved against the markdown file's directory.
- Markdown is converted to HTML with local subprocesses instead of an embedded markdown library.
- The parser strategy is deliberately ugly but pragmatic. If this grows later, replace the subprocess parser path with a real markdown library.
- Each running instance exposes its own local Qt socket and writes a tiny runtime registry entry so `--pid` and the introspection commands can target the right window.
- For project boundaries, see [DESIGN_SCOPE.md](./DESIGN_SCOPE.md).
- Release history lives in [CHANGELOG.md](./CHANGELOG.md).
- Licensing lives in [LICENSE](./LICENSE).
