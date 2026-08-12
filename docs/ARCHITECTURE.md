# Architecture

`ofxGgmlAgents` owns agent-specific orchestration code and has no direct ggml
runtime dependency. Concrete model execution stays in its companion addon.

## Dependency Direction

```text
openFrameworks app
  -> ofxGgmlAgents
  -> optional retrieval and model companions
```

Tool and model handoffs remain app-level composition; Agents does not link
model companions transitively.

## Owned Here

- agent request/result helpers
- tool registry and tool-call adapters
- planning loop boundaries
- focused root-level examples
- local model/tool workflow documentation

See `docs/AGENT_WORKFLOWS.md` for the planning handoff and workflow-boundary
rules used before source-level agent behavior is expanded.

## Not Owned Here

- ggml runtime setup and backend selection
- generic tensor, graph, model metadata, and result types
- text, vision, audio, video, RAG, or diffusion-specific workflows
