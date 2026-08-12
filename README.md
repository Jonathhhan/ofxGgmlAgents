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
- allowlisted local RAG search with cited results
- selectable CPU/CUDA endpoint profiles with visible chosen backend
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
commands, and explicit out-of-scope runtime work. Its Endpoint tab can check the
selected endpoint, run the ecosystem manifest tool, or search a user-selected
local corpus through `ofxGgmlRag`. Generate it with the openFrameworks projectGenerator using addons
`ofxGgmlAgents`, `ofxGgmlCore`, `ofxGgmlRag`, and `ofxImGui`.

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
- `ofxGgmlRag` and `ofxImGui` for the combined example only

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

The default allowlisted tool is `get_proven_lanes`; it reads the canonical sibling
`ofxGgmlWorkflows/ecosystem.yaml`. The combined openFrameworks example also offers
`search_local_corpus`, which delegates retrieval and citations to `ofxGgmlRag`.
The corpus root is selected by the user and is never accepted from model output;
the model may supply only the focused query. Override manifest discovery with
`-EcosystemPath` or `OFXGGML_AGENT_ECOSYSTEM_PATH`. The runner accepts both
OpenAI `tool_calls` and a JSON `{name, arguments}` object in assistant content.
It does not write the manifest or expose arbitrary filesystem access.

The example keeps separate CPU and CUDA endpoint profiles. Configure them with
`OFXGGML_AGENT_CPU_LLM_BASE_URL` / `OFXGGML_AGENT_CPU_LLM_MODEL` and
`OFXGGML_AGENT_CUDA_LLM_BASE_URL` / `OFXGGML_AGENT_CUDA_LLM_MODEL`, or edit the
active fields in the UI. The displayed backend is the selected configuration
profile: CPU means zero model layers offloaded (`-GpuLayers 0`), while CUDA
means GPU layer offload (`-GpuLayers all`). A CUDA-enabled llama.cpp binary may
still initialize its CUDA runtime in the CPU profile. `ofxGgmlLlama` remains
responsible for starting the matching server.

The legacy `OFXGGML_AGENT_LLM_BASE_URL` fallback is intentionally shown as a
shared endpoint with unknown offload instead of being claimed as both CPU and
CUDA. Use the profile-specific variables for meaningful switching. Model POSTs
time out after 120 seconds by default; set
`OFXGGML_AGENT_LLM_TIMEOUT_SECONDS` to a value from 1 through 3600 when needed.

`Check selected endpoint` explicitly queries the chosen profile's OpenAI-compatible
`/v1/models` route and confirms that its configured model alias is advertised.
The UI reports readiness, HTTP status, elapsed time, and the first meaningful
error without starting a provider or silently falling back to the other profile.
It also lists the aliases actually advertised by the selected server and can
copy the first advertised alias into the active profile after a mismatch.

The backend name remains configuration evidence: current `llama-server`
`/health`, `/v1/models`, and `/props` responses do not expose the GPU-layer
offload value. The example therefore does not relabel a profile from timing or
process heuristics.

The local RAG result panel displays the immutable query used by the worker,
bounded retrieved excerpts, and their cited source references.

## Boundary

Keep agent-specific planning, tool registry, orchestration, memory handoff,
provider endpoint handoff, and examples here. Model launch and model-specific
runtime setup stay in the owning companion addon. Move code down into
`ofxGgmlCore` only when it becomes a stable, domain-neutral primitive with
focused tests.

`ofxGgmlAgentsPlannerExample` provides the interactive counterpart to the
runtime smoke: its Endpoint tab explicitly checks the selected local endpoint,
then can run either the read-only `get_proven_lanes` loop or cited
`search_local_corpus` loop while remaining offline until clicked.
