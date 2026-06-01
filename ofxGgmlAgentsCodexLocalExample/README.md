# ofxGgmlAgentsCodexLocalExample

This folder is a handoff pointer, not an `ofxGgmlAgents` runtime example.

The concrete OpenAI Codex + llama.cpp setup belongs in
`ofxGgmlLlama/ofxGgmlLlamaCodexLocalExample`, where the owning addon can show
model discovery, server startup, provider configuration, and runtime validation
together.

Use `ofxGgmlAgentsPlannerExample` in this addon to record the planning side of
that handoff:

- workflow goal
- endpoint base URL and model alias
- companion addon ownership
- out-of-scope runtime work
- validation commands

Generated Visual Studio project files, binaries, caches, sessions, model files,
and local provider configuration should stay out of git.
