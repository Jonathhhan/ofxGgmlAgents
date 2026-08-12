# Local LLM Endpoint Handoff

`ofxGgmlAgents` consumes local model endpoints. It does not own llama.cpp
builds, GGUF downloads, model caches, or server lifecycle.

Use this guide when Codex, GitHub Copilot, Hermes Agent, or another local
assistant is backed by a llama.cpp `llama-server` prepared by `ofxGgmlLlama`.

## Ownership split

| Concern | Owner |
| --- | --- |
| llama.cpp build and install | `ofxGgmlLlama` |
| GGUF model download and discovery | `ofxGgmlLlama` |
| `llama-server` startup and health checks | `ofxGgmlLlama` |
| OpenAI-compatible endpoint URL and model alias | handoff input to `ofxGgmlAgents` |
| agent planning loop, tool registry, and handoff records | `ofxGgmlAgents` |
| companion-addon tools used by an agent | owning companion addon |

The Llama setup guide is maintained in
[`ofxGgmlLlama/docs/CODEX_COPILOT_LOCAL_SERVER.md`](https://github.com/Jonathhhan/ofxGgmlLlama/blob/main/docs/CODEX_COPILOT_LOCAL_SERVER.md).

## Expected endpoint contract

Agents may assume an OpenAI-compatible base URL and model alias:

```text
base_url: http://127.0.0.1:8001/v1
model: unsloth/GLM-4.7-Flash
```

The backing server should expose chat/completion endpoints compatible with the
client being used by the coding assistant. Agents docs and examples should avoid
hardcoding model paths or llama.cpp binary locations.

The concrete OpenAI Codex + llama.cpp setup example is maintained in
`ofxGgmlLlama/ofxGgmlLlamaCodexLocalExample`, where the owning addon can show
build, model, server, and Codex provider/profile setup together. Agents docs
should reference that example instead of duplicating llama.cpp runtime setup.

## Handoff record

Use this template before an agent workflow depends on a local model:

```text
Workflow:
Assistant client:
Endpoint base URL:
Model alias:
Provider owner: ofxGgmlLlama
Server health checked:
Tool registry needed:
Companion tools needed:
Generated artifacts:
Cleanup rules:
Validation:
```

Example:

```text
Workflow: local coding-agent planning
Assistant client: Codex-compatible OpenAI client
Endpoint base URL: http://127.0.0.1:8001/v1
Model alias: unsloth/GLM-4.7-Flash
Provider owner: ofxGgmlLlama
Server health checked: yes, via llama-server /health
Tool registry needed: none for planning-only smoke
Companion tools needed: ofxGgmlLlama for text generation
Generated artifacts: none in ofxGgmlAgents
Cleanup rules: stop server from ofxGgmlLlama scripts
Validation: scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly
```

## Environment names

Until runtime provider code exists in this addon, prefer documenting endpoint
handoff values rather than adding new runtime behavior. When provider runtime is
introduced, use explicit environment names:

```text
OFXGGML_AGENT_LLM_BASE_URL=http://127.0.0.1:8001/v1
OFXGGML_AGENT_LLM_MODEL=unsloth/GLM-4.7-Flash
```

The planner example additionally supports two named profiles:

```text
OFXGGML_AGENT_CPU_LLM_BASE_URL=http://127.0.0.1:8082
OFXGGML_AGENT_CPU_LLM_MODEL=local-qwen-cpu
OFXGGML_AGENT_CUDA_LLM_BASE_URL=http://127.0.0.1:8080
OFXGGML_AGENT_CUDA_LLM_MODEL=local-qwen-cuda
```

The generic values remain compatibility fallbacks for both profiles. When such
a shared fallback is active, the example labels its offload as unknown; it does
not claim that selecting CPU or CUDA changes the underlying server. Configure
profile-specific URLs for meaningful switching. The profile label is
configuration evidence, not automatic hardware detection; verify the owning
`llama-server` command separately (`-GpuLayers 0` for CPU, `-GpuLayers all` for
CUDA). A CUDA-enabled binary may initialize its CUDA runtime even with zero
model layers offloaded; that does not turn the CPU profile into GPU inference.

Model generation requests have a finite 120-second timeout. Set
`OFXGGML_AGENT_LLM_TIMEOUT_SECONDS` to an integer from 1 through 3600 to adjust
it. Endpoint discovery keeps its shorter five-second timeout.

In the planner example, use `Check selected endpoint` to query the selected
profile's `/v1/models` route. The check is explicit so browsing the UI remains
offline. It succeeds only when the endpoint is reachable and advertises the
configured model alias; it does not fall back from CPU to CUDA or vice versa.
The result lists the aliases reported by `/v1/models`; after a mismatch the UI
can explicitly copy the first advertised alias into the selected profile.

Current `llama-server` responses from `/health`, `/v1/models`, and `/props` do
not report GPU layer offload. Do not infer CPU or CUDA from request timing. The
selected backend label therefore remains launch-configuration evidence, while
endpoint reachability and advertised models are separately observed evidence.

Keep text-generation-specific server management in `ofxGgmlLlama`. Agents code
should treat these values as an already-provisioned provider.

## Validation boundary

For planning-only agent work:

```powershell
scripts\doctor-agents.bat
scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly
scripts\validate-local.bat
```

For real model-serving validation, run the Llama lane first:

```powershell
cd ..\ofxGgmlLlama
scripts\doctor-llama.bat
scripts\list-models.bat
scripts\run-llama-runtime-smoke.bat -Backend cuda -Json -SummaryOnly
```

Use `-Backend cpu` for CPU-only validation. Only after that should an agent
workflow claim model-backed local LLM readiness.

You can additionally validate the agent smoke contract directly from this lane:

```powershell
cd ..\ofxGgmlAgents
set OFXGGML_AGENT_LLM_BASE_URL=http://127.0.0.1:8001/v1
set OFXGGML_AGENT_LLM_MODEL=unsloth/GLM-4.7-Flash
scripts\run-agents-runtime-smoke.bat -Json -SummaryOnly -OutputPath .agents-runtime-smoke.json
```

When the endpoint returns the smoke token, Core will classify this lane as
`inference-checked` in `plan-backend-runtime-verification`.
