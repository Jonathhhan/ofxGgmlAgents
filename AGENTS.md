# AGENTS.md

This repository is `ofxGgmlAgents`, the local-agent and planning companion addon for the ofxGgml family.

Codex should treat `ofxGgmlCore` as the backend-neutral foundation. This repo owns tool-using local agents, planning loops, agent orchestration, local tool execution boundaries, and agent-specific examples.

## Addon contract

Do:

- keep agent/planning-specific workflows in this addon
- depend on shared primitives from `ofxGgmlCore`
- preserve openFrameworks addon layout and `addon_config.mk`
- keep examples projectGenerator-friendly
- document tool/runtime boundaries clearly

Do not:

- move backend-neutral Core primitives into this repo
- commit generated tool outputs, private data, binaries, or caches
- hardcode local absolute paths
- add unsafe autonomous behavior without explicit user control

## Codex workflow

1. Inspect existing files first.
2. Keep changes small and focused.
3. Preserve addon boundaries.
4. Update docs/examples/scripts with code changes.
5. Summarize validation honestly.
