---
name: slime-doc-nav
description: Use when local markdown documents should be opened, focused, or queried in the SlimeMDViewer app, especially to open a specific file, place it on a chosen monitor, target a running instance by PID, scroll to an anchor, or dump the currently visible text.
---

# Slime Doc Nav

Use this skill when the task benefits from opening a local markdown document in `slime`, focusing the exact section under discussion, or querying the current live document state.

This skill covers four related behaviors:
- `open`: launch `slime` on a specific markdown file
- `scroll`: send a live jump command to an already-running `slime` instance
- `info`: query a running instance for its current file/path/title
- `dump-visible`: read the currently visible document slice from a running instance

## When To Use

Use this skill when:
- a markdown document is the current working context
- the user would benefit from seeing a specific file while you work
- the user would benefit from jumping to a specific section of an already-open file
- the user says things like "look at this", "read what I'm on", or "what does this section mean"
- it would help to inspect a running `slime` instance instead of launching a new one

Do not use this skill for:
- non-markdown files
- editing the markdown itself
- general desktop automation outside `slime`

## Decision Rule

Choose the behavior by intent:

- If the target file is not already open, `open` it.
- If the correct file is already open and only the section focus is wrong, use `scroll`.
- If both are needed, `open` first and then `scroll`.
- If `scroll` is requested but no viewer instance is running, fall back to `open` if the file path is known.
- If the user needs current on-screen context, use `dump-visible`.
- If multiple windows are open, prefer `--pid` instead of guessing.

## Commands

Open a document:

```bash
slime /path/to/file.md
```

Open a document on a specific monitor:

```bash
slime /path/to/file.md --monitor 1
slime /path/to/file.md --monitor 2
```

Scroll an already-running viewer to an anchor:

```bash
slime --scroll anchor-id
```

Scroll a specific running instance:

```bash
slime --pid 12345 --scroll anchor-id
```

Show information for the most recent running instance:

```bash
slime --info
```

Show information for a specific instance:

```bash
slime --pid 12345 --info
```

Dump the currently visible text from the most recent running instance:

```bash
slime --dump-visible
```

Dump the currently visible text from a specific instance:

```bash
slime --pid 12345 --dump-visible
```

List known running instances:

```bash
slime --list-instances
```

Open and then focus a section:

```bash
slime /path/to/file.md --monitor 1
slime --scroll anchor-id
```

## Notes

- `slime --scroll ...`, `--info`, and `--dump-visible` use the viewer's local socket hook and only make sense if a viewer instance is already running.
- `slime --list-instances` is the safest way to discover which PID to target when several windows are open.
- `slime --dump-visible` is the preferred hook for vague prompts like "look at this".
- Anchor names must match the rendered document's anchor ids.
- Keep this skill narrow. It is for document navigation, not note-system management or general GUI control.
