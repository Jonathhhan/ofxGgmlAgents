# Agent Workflow Boundaries

`ofxGgmlAgents` owns local agent orchestration patterns for the ofxGgml
ecosystem. This document is for Codex, GitHub Copilot, Hermes Agent, and
humans planning agent work before addon runtime code exists.

## Owned workflow surface

This addon may define:

- planning request and response shapes
- tool registry concepts and tool-call boundaries
- local execution loop documentation
- example prompts and handoff records
- validation checks for agent workflow structure
- integration notes for companion addons that expose tools

## Not owned here

Keep these responsibilities out of `ofxGgmlAgents`:

- ggml setup, backend selection, and runtime discovery
- text, audio, vision, video, diffusion, segmentation, music, or RAG model UX
- model downloads, model caches, embeddings indexes, or generated media
- generic result, tensor, graph, or metadata primitives that belong in
  `ofxGgmlCore`
- reusable GitHub Actions policy, which belongs in `ofxGgmlWorkflows`

## Planning handoff

Agent workflow changes should start with a planning handoff before any source
changes:

1. State the user-facing workflow being improved.
2. Identify which companion addon would provide each tool or model capability.
3. Mark whether the change is documentation, validation, example scaffolding,
   or runtime behavior.
4. Keep runtime behavior out of scope unless explicitly requested.
5. List the validation command for the repository being changed.

Use this template for planning notes:

```text
Workflow:
User goal:
Repository touched:
Companion tools needed:
Out of scope:
Validation:
```

## Validation ladder

Use the smallest command that proves the changed layer:

| Change type | Suggested validation |
| --- | --- |
| Docs or planning only | `scripts\validate-local.bat` |
| Local setup diagnosis | `scripts\doctor-agents.bat` |
| Request/result/helper changes | `scripts\test-addon.bat` |
| Ecosystem runtime smoke evidence | `scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly` |
| Example layout changes | `scripts\validate-local.bat` |

`scripts\run-agents-runtime-smoke.*` is intentionally planning-boundary-only
until this addon owns a real local model runner, tool registry, execution loop,
or memory handoff. It compiles and runs the deterministic helper tests, checks
doctor readiness, and emits JSON for Core planning without downloading models,
running tools, mutating memory indexes, or invoking companion-addon workflows.

## Safe first tasks

Good early tasks for this lane are:

- documenting tool-call contracts
- adding validation around example structure
- improving README handoff guidance
- describing how a companion addon can expose a tool to an agent
- adding small planning examples that do not execute local models

Avoid expanding runtime behavior until the planning and validation surface is
clear enough for repeatable review.
