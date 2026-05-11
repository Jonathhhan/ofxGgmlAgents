# Architecture

`ofxGgmlAgents` owns agent-specific orchestration code. It should use `ofxGgmlCore` for stable runtime primitives and keep workflow loops out of core.

## Dependency Direction

```text
openFrameworks app
  -> ofxGgmlAgents
      -> ofxGgmlCore
```

No dependency should point from `ofxGgmlCore` back to `ofxGgmlAgents`.

## Owned Here

- agent request/result helpers
- tool registry and tool-call adapters
- planning loop boundaries
- focused root-level examples
- local model/tool workflow documentation

## Not Owned Here

- ggml runtime setup and backend selection
- generic tensor, graph, model metadata, and result types
- text, vision, audio, video, RAG, or diffusion-specific workflows
