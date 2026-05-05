# AGENTS.md

## Purpose

This project is a small local markdown viewer for human/AI collaboration.

Agents working in this repo should preserve that scope.
Use [DESIGN_SCOPE.md](./DESIGN_SCOPE.md) as the primary product-boundary reference.

## Core Rule

Keep `slime` a sharp tool, not a platform.

Do not evolve it into:

- a note-taking suite
- a sync service
- a plugin ecosystem
- a browser replacement
- a full editor

## Implementation Bias

Prefer:

- direct, readable Qt code
- local-first behavior
- simple file-oriented workflows
- small features that reduce real friction
- explicit control paths over hidden machinery

Be suspicious of:

- abstraction added for its own sake
- metadata systems without real pressure
- heavy framework churn
- architecture that tries to predict a distant future

## Good Changes

Good changes usually:

- improve document viewing directly
- improve navigation or control of the current document
- improve human/AI collaborative use
- keep the code understandable
- solve a problem already observed in use

Examples:

- better instance targeting
- tabs
- packaging/install cleanup
- small UI quality-of-life improvements

## Bad Changes

Bad changes usually:

- push `slime` toward becoming a larger product category
- add complex state management without clear payoff
- introduce heavyweight persistence or indexing too early
- expand scope faster than proven usage justifies

Examples:

- embedded database architecture
- cloud/account features
- giant note metadata systems
- complex editor behavior
- Obsidian-like feature creep

## Working Style

When making changes:

- prefer small, testable increments
- preserve direct launch-and-verify workflows
- test real runtime behavior after meaningful UI/control changes
- document new control surfaces in the README

If a feature idea feels useful but too expansive, capture it as a note instead of forcing it into the code immediately.

## Current Known Direction

The current repo already supports:

- opening local markdown files
- auto-reload on file change
- anchor scrolling
- monitor placement
- per-instance PID targeting for live control
- runtime info queries
- visible-text dumping for collaboration workflows
- local install/uninstall scripts

Future work should build from that concrete base rather than inventing a second system around it.
