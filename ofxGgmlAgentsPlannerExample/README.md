# ofxGgmlAgentsPlannerExample

Root-level planning smoke example for `ofxGgmlAgents`.

The example is intentionally model-free. It shows how this addon records a
planning request, companion-tool ownership, out-of-scope runtime work, and the
validation command before any model-backed agent loop is added.

## What it demonstrates

- `ofxGgmlAgentsRequest` fields for goal, prompt, context, and companion tools
- the workflow handoff template from `docs/AGENT_WORKFLOWS.md`
- clear separation between `ofxGgmlAgents` planning and companion-addon runtime
  ownership
- openFrameworks logging through `ofLogNotice`
- copyable handoff records for issue, PR, or planning notes
- local LLM endpoint handoff status from `OFXGGML_AGENT_LLM_BASE_URL` and
  `OFXGGML_AGENT_LLM_MODEL`

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

## Validate

From the addon root:

```powershell
scripts\validate-local.bat
```
