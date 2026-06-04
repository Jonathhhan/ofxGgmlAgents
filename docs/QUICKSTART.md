# Quickstart

Use this path from a fresh openFrameworks checkout when you want to inspect or
run the `ofxGgmlAgents` planning example.

## 1. Clone the addon lane

From the openFrameworks `addons` folder:

```powershell
git clone https://github.com/Jonathhhan/ofxGgmlCore.git
git clone https://github.com/Jonathhhan/ofxGgmlAgents.git
git clone https://github.com/jvcleave/ofxImGui.git
```

The planner example also expects `ofxImGui` beside these addons:

```text
addons/
  ofxGgmlCore/
  ofxGgmlAgents/
  ofxImGui/
```

## 2. Validate the planning lane

From `addons/ofxGgmlAgents`:

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

These checks prove the addon skeleton, planning request/helper contract, example
handoff files, and generated-artifact hygiene. They do not start local models or
execute tools.

## 3. Generate the planner example

Use the openFrameworks projectGenerator for
`ofxGgmlAgentsPlannerExample`, or refresh the checked-in example project, with
these addons:

```text
ofxGgmlAgents
ofxGgmlCore
ofxImGui
```

Keep the project at the addon root, not under a nested `examples/` folder. Build
and run the generated project. The example lets you switch between planning
scenarios, copy or log a handoff record, and inspect local endpoint environment
variables without making network requests.

## 4. Hand off local LLM setup

If a workflow needs a local OpenAI-compatible endpoint, keep server setup in
`ofxGgmlLlama`. This addon should only record handoff values such as:

```text
OFXGGML_AGENT_LLM_BASE_URL=http://127.0.0.1:8001/v1
OFXGGML_AGENT_LLM_MODEL=unsloth/GLM-4.7-Flash
```

See `docs/LOCAL_LLM_ENDPOINTS.md` for the boundary and
`ofxGgmlLlama/ofxGgmlLlamaCodexLocalExample` for the concrete Codex + llama.cpp
setup.

## 5. Keep local outputs out of git

Do not commit generated Visual Studio projects, binaries, model files, downloaded
runtimes, memory indexes, provider credentials, or cache folders. The validation
script checks the main generated paths before handoff.
