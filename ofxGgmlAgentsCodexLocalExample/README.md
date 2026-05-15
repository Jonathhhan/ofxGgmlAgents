# ofxGgmlAgentsCodexLocalExample

Root-level handoff example for using Codex with a local OpenAI-compatible
`llama-server` endpoint prepared by `ofxGgmlLlama`.

This example does not start llama.cpp, download models, edit Codex config, or
run an agent loop. It shows the endpoint contract and the Codex provider/profile
snippet that should point at an already-running local server.

## Run

1. Start a local coding-agent server from `ofxGgmlLlama`.

   ```powershell
   cd ..\ofxGgmlLlama
   scripts\start-llama-server.bat -ModelPath C:\path\to\model.gguf -Port 8001 -GpuLayers 999 -ContextSize 131072
   ```

2. Optionally set the endpoint values before launching this example.

   ```powershell
   $env:OFXGGML_AGENT_LLM_BASE_URL = "http://127.0.0.1:8001/v1"
   $env:OFXGGML_AGENT_LLM_MODEL = "unsloth/GLM-4.7-Flash"
   ```

3. Generate this example with openFrameworks projectGenerator using addons
   `ofxGgmlAgents`, `ofxGgmlCore`, and `ofxImGui`.

4. Copy the shown provider/profile shape into your local Codex config after
   verifying it against your installed Codex version. This folder also includes
   `codex-config.example.toml` as a concrete starting point.

## Validate

From `ofxGgmlCore`, the local Codex readiness planner checks config and endpoint
visibility without mutating files:

```powershell
scripts\plan-local-codex.bat -Endpoint http://127.0.0.1:8001/v1 -Json -SummaryOnly
```

Keep model weights, downloaded runtimes, generated project files, logs, and
local Codex config out of git.
