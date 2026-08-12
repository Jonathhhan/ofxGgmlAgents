# ofxGgmlAgentsPlannerExample

Root-level planning smoke example for `ofxGgmlAgents`.

The example remains offline by default. It records planning boundaries and can
also run one explicit, model-backed, read-only tool loop after the user clicks
`Run allowlisted tool loop`.

## What it demonstrates

- `ofxGgmlAgentsRequest` fields for goal, prompt, context, and companion tools
- the workflow handoff template from `docs/AGENT_WORKFLOWS.md`
- clear separation between `ofxGgmlAgents` planning and companion-addon runtime
  ownership
- openFrameworks logging through `ofLogNotice`
- copyable handoff records for issue, PR, or planning notes
- local LLM endpoint handoff status from `OFXGGML_AGENT_LLM_BASE_URL` and
  `OFXGGML_AGENT_LLM_MODEL`
- editable OpenAI-compatible endpoint and model fields
- separate CPU and CUDA endpoint profiles, an explicit selector, visible
  selected/last-executed backend labels, and an explicit readiness check
- an allowlisted `get_proven_lanes` request, canonical manifest read, returned
  lane list, final `OFXGGML_AGENTS_TOOL_OK`, errors, and elapsed time
- an allowlisted `search_local_corpus` request backed directly by `ofxGgmlRag`,
  with a folder browser, model-selected query, cited sources, and bounded context
- normalization of structured `tool_calls` and JSON tool requests in assistant
  content, matching `scripts/run-agents-runtime-smoke.ps1`

## Run

Generate or refresh the project with openFrameworks projectGenerator using:

```text
ofxGgmlAgents
ofxGgmlCore
ofxGgmlRag
ofxImGui
```

Then build and run `ofxGgmlAgentsPlannerExample`. The UI contains sample
planning scenarios plus `Log handoff` and `Copy handoff` actions for the
selected record. The Endpoint tab shows whether the local LLM handoff
environment is configured, but it does not start a server or download models.
Network requests occur only after `Check selected endpoint` or `Run allowlisted
tool loop` is clicked. For the local endpoint scenario, the copied handoff
record includes the configured base URL and model alias while keeping API key
values hidden.

The Endpoint tab does nothing until the user clicks `Check selected endpoint`
or `Run allowlisted tool loop`. The check queries `/v1/models`, requires the
configured model alias to be advertised, and shows HTTP status, elapsed time,
advertised aliases, and errors. When an alias is wrong, `Use first advertised
model` copies the server's first alias into the selected profile for an explicit
recheck. The example calls only the already-running endpoint; provider startup
and downloads remain owned by `ofxGgmlLlama`. The executable
tools are `get_proven_lanes`, which reads the canonical sibling
`ofxGgmlWorkflows/ecosystem.yaml`, and `search_local_corpus`, which uses
`ofxGgmlRag` to return cited local evidence. The user selects the corpus folder;
the model cannot change that root and may provide only a query of up to 512
characters. Set `OFXGGML_AGENT_ECOSYSTEM_PATH` or
`OFXGGML_AGENT_RAG_SOURCE_ROOT` to override the initial paths.

CPU and CUDA are endpoint profiles rather than runtime autodetection. The
example uses `OFXGGML_AGENT_CPU_LLM_BASE_URL` and
`OFXGGML_AGENT_CUDA_LLM_BASE_URL` (plus matching `_MODEL` variables), preserves
both profiles while switching, and shows which one was selected and executed.
CPU denotes zero model layers offloaded (`-GpuLayers 0`); a CUDA-enabled server
binary can still initialize a CUDA runtime context. CUDA denotes actual model
layer offload (`-GpuLayers all`). Start those endpoints with the corresponding
`ofxGgmlLlama` server settings. The server's `/health`, `/v1/models`, and
`/props` responses do not report GPU layer offload, so the displayed backend
remains configuration evidence rather than inferred hardware detection.

If only the legacy `OFXGGML_AGENT_LLM_BASE_URL` is set, both selectors point to
one shared endpoint and are labelled `offload unknown`; use profile-specific
URLs for a real CPU/CUDA switch. Model requests use a 120-second timeout by
default. Override it with `OFXGGML_AGENT_LLM_TIMEOUT_SECONDS` from 1 to 3600.
The RAG result shows the immutable executed query, bounded retrieved excerpts,
and their cited sources.

## Validate

From the addon root:

```powershell
scripts\validate-local.bat
```

After building, the same compiled example path can be exercised without opening
the GUI:

```powershell
ofxGgmlAgentsPlannerExample\bin\ofxGgmlAgentsPlannerExample.exe --endpoint-check-smoke cpu http://127.0.0.1:8082 local-qwen-cpu
ofxGgmlAgentsPlannerExample\bin\ofxGgmlAgentsPlannerExample.exe --tool-loop-smoke http://127.0.0.1:11434 qwen2.5-coder:7b ..\ofxGgmlWorkflows\ecosystem.yaml
ofxGgmlAgentsPlannerExample\bin\ofxGgmlAgentsPlannerExample.exe --backend-tool-loop-smoke cuda http://127.0.0.1:8080 local-qwen-cuda ..\ofxGgmlWorkflows\ecosystem.yaml
ofxGgmlAgentsPlannerExample\bin\ofxGgmlAgentsPlannerExample.exe --rag-tool-loop-smoke http://127.0.0.1:11434 qwen2.5-coder:7b C:\path\to\notes "tool orchestration"
```

These commands emit machine-readable JSON. The endpoint check exits nonzero
unless the selected server is reachable and advertises the configured alias;
tool loops additionally require the complete model, tool, result, and
confirmation path to succeed.
