# ofxGgmlAgents

`ofxGgmlAgents` is the companion addon for local agent orchestration, tool-use loops, planning helpers, and assistant workflows on top of `ofxGgmlCore`.

`ofxGgmlCore` stays the dependency. This addon owns agent-specific workflow code so core can stay small and boring.

Family map: https://jonathhhan.github.io/ofxGgmlCore/

Current addon API version: `1.0.1`.

## First Milestone

- define small request/result types
- keep one root-level smoke example
- keep generated models, media, builds, and IDE files out of git
- validate the addon with local headless tests

## Example

`ofxGgmlAgentsPlannerExample` is a root-level planning request smoke test. Generate it with the openFrameworks projectGenerator using addons `ofxGgmlAgents`, `ofxGgmlCore`, and `ofxImGui`.

## Agent workflow planning

Use [`docs/AGENT_WORKFLOWS.md`](docs/AGENT_WORKFLOWS.md) before expanding this
lane. It defines which planning, tool-use, and handoff responsibilities belong
in `ofxGgmlAgents`, which model-specific behavior stays in companion addons,
and how Codex, GitHub Copilot, or Hermes Agent should scope a repository
change.

## Dependencies

- openFrameworks
- `ofxGgmlCore`
- `ofxImGui` for examples

## Validate

```powershell
scripts\doctor-agents.bat
scripts\validate-local.bat
```

On macOS/Linux:

```sh
./scripts/doctor-agents.sh
./scripts/validate-local.sh
```

## Boundary

Keep agent-specific planning, tool registry, orchestration, memory handoff, model launch, and examples here. Move code down into `ofxGgmlCore` only when it becomes a stable, domain-neutral primitive with focused tests.
