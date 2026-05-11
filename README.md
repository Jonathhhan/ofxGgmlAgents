# ofxGgmlAgents

`ofxGgmlAgents` is the companion addon for local agent orchestration, tool-use loops, planning helpers, and assistant workflows on top of `ofxGgmlCore`.

`ofxGgmlCore` stays the dependency. This addon owns agent-specific workflow code so core can stay small and boring.

Family map: https://jonathhhan.github.io/ofxGgmlCore/

## First Milestone

- define small request/result types
- keep one root-level smoke example
- keep generated models, media, builds, and IDE files out of git
- validate the addon with local headless tests

## Example

`ofxGgmlAgentsPlannerExample` is a root-level planning request smoke test. Generate it with the openFrameworks projectGenerator using addons `ofxGgmlAgents` and `ofxGgmlCore`.

## Dependencies

- openFrameworks
- `ofxGgmlCore`

## Validate

```powershell
scripts\validate-local.bat
```

On macOS/Linux:

```sh
./scripts/validate-local.sh
```

## Boundary

Keep agent-specific planning, tool registry, orchestration, memory handoff, model launch, and examples here. Move code down into `ofxGgmlCore` only when it becomes a stable, domain-neutral primitive with focused tests.
