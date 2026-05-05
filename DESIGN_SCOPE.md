# Slime Design Scope

## What Slime Is

Slime is a small local markdown viewer for human/AI collaboration.

Its job is to:

- open local markdown documents quickly
- render them readably without a full note suite
- reload when files change
- support lightweight navigation and focus commands
- support human/AI collaboration with explicit control hooks and a matching agent skill
- stay simple enough to understand and modify easily

The matching skill should stay narrow too.
It should help agents:

- open the right markdown file
- target the right running instance
- scroll or focus the right section
- query visible document context

It should not become a general desktop automation layer.

## What Slime Should Optimize For

- low friction
- local-first use
- direct file access
- simple control hooks
- good-enough readability
- easy portability of the core idea

## What Slime Is Not

Slime is not meant to become:

- a note-taking suite
- a knowledge graph app
- a sync platform
- a plugin ecosystem
- a general browser replacement
- a full WYSIWYG editor

## Scope Guardrails

Features are good fits when they:

- improve document viewing directly
- improve human/AI navigation of the current document
- reduce friction in an already-real workflow
- keep the implementation understandable

Features are bad fits when they:

- add heavy management layers around notes
- introduce framework-scale complexity without clear payoff
- try to make Slime own the entire notes workflow
- solve hypothetical future problems instead of current friction

## Good Near-Term Directions

- tabs
- small usability improvements
- better document/context introspection
- cleaner window behavior as the app grows

## Bad Near-Term Directions

- embedded database architecture
- account/cloud features
- giant metadata systems
- complex editor behavior
- turning it into Obsidian-lite

## Working Rule

Slime should stay a sharp tool, not a platform.
