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

`ofxGgmlAgentsCodexLocalExample` shows this handoff as a projectGenerator-ready
openFrameworks example. It reads `OFXGGML_AGENT_LLM_BASE_URL` and
`OFXGGML_AGENT_LLM_MODEL`, then displays the endpoint and Codex provider/profile
shape without starting servers or editing local config.

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
