# ofxGgmlAgents

`ofxGgmlAgents` is the companion addon for local agent orchestration, tool-use loops, planning helpers, and assistant workflows on top of `ofxGgmlCore`.

`ofxGgmlCore` stays the dependency. This addon owns agent-specific workflow code so core can stay small and boring.

This is the internal agent lane for the ofxGgml ecosystem: planning loops, tool
registries, memory handoff, and addon-to-addon orchestration live here. Llama.cpp
server startup, GGUF model discovery, and client-specific Codex/OpenCode config
snippets remain in `ofxGgmlLlama`, where the local text endpoint is owned.

Family map: https://jonathhhan.github.io/ofxGgmlCore/

Current addon API version: `1.0.1`.

## Features

- planning-boundary runtime smoke
- Hermes Agent environment detection
- tool orchestration workflow boundary
- OpenAI-compatible endpoint smoke option
- local coding-agent handoff planning

## First Milestone

- define small request/result types
- keep one root-level smoke example
- keep generated models, media, builds, and IDE files out of git
- validate the addon with local headless tests

## Example

`ofxGgmlAgentsPlannerExample` is a root-level planning request smoke test. It
shows sample agent workflow handoffs, companion-tool ownership, validation
commands, and explicit out-of-scope runtime work without starting local models
or executing tools. Generate it with the openFrameworks projectGenerator using
addons `ofxGgmlAgents`, `ofxGgmlCore`, and `ofxImGui`.

## Agent workflow planning

For a fresh checkout, see [`docs/QUICKSTART.md`](docs/QUICKSTART.md). It covers
the clone, validation, projectGenerator, and model-handoff path from a new
user's point of view.

Use [`docs/AGENT_WORKFLOWS.md`](docs/AGENT_WORKFLOWS.md) before expanding this
lane. It defines which planning, tool-use, and handoff responsibilities belong
in `ofxGgmlAgents`, which model-specific behavior stays in companion addons,
and how Codex, GitHub Copilot, or Hermes Agent should scope a repository
change.

For local LLM provider handoff from an `ofxGgmlLlama` `llama-server`, see
[`docs/LOCAL_LLM_ENDPOINTS.md`](docs/LOCAL_LLM_ENDPOINTS.md).
The concrete OpenAI Codex + llama.cpp setup example lives in
`ofxGgmlLlama/ofxGgmlLlamaCodexLocalExample`.

Hermes Agent is treated as an external planning client. If it is installed at
`%LOCALAPPDATA%\hermes` or `HERMES_HOME`, `scripts\doctor-agents.*` reports the
Hermes root, agent checkout, config, and memory-store presence while keeping
Hermes sessions, memories, skills, logs, caches, API keys, and model artifacts
outside git. See [`docs/AGENT_WORKFLOWS.md`](docs/AGENT_WORKFLOWS.md) for the
Hermes handoff template.

## Dependencies

- openFrameworks
- `ofxGgmlCore`
- `ofxImGui` for examples

## Validate

```powershell
scripts\doctor-agents.bat
scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly
scripts\validate-local.bat
```

On macOS/Linux:

```sh
./scripts/doctor-agents.sh
./scripts/run-agents-runtime-smoke.sh -Json -SummaryOnly
./scripts/validate-local.sh
```

`scripts\run-agents-runtime-smoke.*` is the lane-owned runtime-smoke entrypoint
for ecosystem planning and CI rollouts. Its default behavior continues to prove
the deterministic planning request/helper boundary and doctor readiness. An
explicit opt-in proves one model-backed, read-only tool loop against an already
running OpenAI-compatible endpoint:

```powershell
scripts\run-agents-runtime-smoke.bat -ServerBaseUrl http://127.0.0.1:11434 -Model qwen2.5-coder:7b -EnableTools -RequireEndpoint -RequireToolExecution -Json -SummaryOnly
```

The only allowlisted tool is `get_proven_lanes`; it reads the canonical sibling
`ofxGgmlWorkflows/ecosystem.yaml`. Override discovery with
`-EcosystemPath` or `OFXGGML_AGENT_ECOSYSTEM_PATH`. The runner accepts both
OpenAI `tool_calls` and a JSON `{name, arguments}` object in assistant content.
It does not write the manifest or expose arbitrary filesystem access.

## Boundary

Keep agent-specific planning, tool registry, orchestration, memory handoff,
provider endpoint handoff, and examples here. Model launch and model-specific
runtime setup stay in the owning companion addon. Move code down into
`ofxGgmlCore` only when it becomes a stable, domain-neutral primitive with
focused tests.

`ofxGgmlAgentsPlannerExample` provides the interactive counterpart to the
runtime smoke: its Endpoint tab explicitly runs the same single allowlisted,
read-only `get_proven_lanes` loop while remaining offline until clicked.
