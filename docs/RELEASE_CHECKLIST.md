# Release Checklist

Use this before tagging or announcing an `ofxGgmlAgents` release. The goal is to
prove the addon boundary and example layout without claiming an agent runtime
that is not wired yet.

## Fresh Clone Layout

From the openFrameworks `addons` folder:

```powershell
git clone https://github.com/Jonathhhan/ofxGgmlCore.git
git clone https://github.com/Jonathhhan/ofxGgmlAgents.git
cd ofxGgmlAgents
```

Expected layout:

```text
addons/
  ofxGgmlCore/
  ofxGgmlAgents/
  ofxImGui/
```

## Local Validation

Run:

```powershell
scripts\validate-local.bat
```

macOS/Linux:

```sh
./scripts/validate-local.sh
```

For a pre-tag release candidate gate:

```powershell
scripts\release-candidate.bat
```

macOS/Linux:

```sh
./scripts/release-candidate.sh
```

The release readiness score should stay lane-specific. It should check Agents
docs, doctor/runtime-smoke scripts, and example handoff files instead of
requiring llama.cpp build scripts or text-generation examples owned by companion
addons.

## Example Scope

`ofxGgmlAgentsPlannerExample` is intentionally narrow in this release:

- root-level openFrameworks example
- `ofxImGui` dependency declared in `addons.make`
- planning request smoke surface with selectable workflow scenarios
- copyable and loggable planning handoff records
- explicit companion-tool ownership and out-of-scope runtime work
- clear future path for local models, tools, memory, and orchestration loops

This release does not promise a complete model-backed agent runtime.

## Before Tagging

- `git status --short --ignored` shows no unexpected generated outputs
- no model files, generated memory/index files, generated OF project files, or
  build outputs are staged
- `CHANGELOG.md` has an entry for the release
- `docs/releases/vX.Y.Z.md` matches the release scope
- release notes distinguish request/helper skeleton work from future runtime
  adapters
