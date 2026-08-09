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
- an allowlisted `get_proven_lanes` request, canonical manifest read, returned
  lane list, final `OFXGGML_AGENTS_TOOL_OK`, errors, and elapsed time
- normalization of structured `tool_calls` and JSON tool requests in assistant
  content, matching `scripts/run-agents-runtime-smoke.ps1`

## Run

Generate or refresh the project with openFrameworks projectGenerator using:

```text
ofxGgmlAgents
ofxGgmlCore
ofxImGui
```

Then build and run `ofxGgmlAgentsPlannerExample`. The UI contains sample
planning scenarios plus `Log handoff` and `Copy handoff` actions for the
selected record. The Endpoint tab shows whether the local LLM handoff
environment is configured, but it does not start a server, download models, or
make network requests. For the local endpoint scenario, the copied handoff
record includes the configured base URL and model alias while keeping API key
values hidden.

The Endpoint tab does nothing until the user clicks `Run allowlisted tool
loop`. The example then calls the already-running endpoint; provider startup,
model discovery, and downloads remain owned by `ofxGgmlLlama`. The only
executable tool is `get_proven_lanes`, which reads the canonical sibling
`ofxGgmlWorkflows/ecosystem.yaml`. Set `OFXGGML_AGENT_ECOSYSTEM_PATH` only when
the automatic sibling path is not suitable.

## Validate

From the addon root:

```powershell
scripts\validate-local.bat
```

After building, the same compiled example path can be exercised without opening
the GUI:

```powershell
ofxGgmlAgentsPlannerExample\bin\ofxGgmlAgentsPlannerExample.exe --tool-loop-smoke http://127.0.0.1:11434 qwen2.5-coder:7b ..\ofxGgmlWorkflows\ecosystem.yaml
```

This emits machine-readable JSON and exits nonzero unless the complete model,
tool, result, and confirmation loop succeeds.
