# Roadmap

## Current Milestone

- Seed the companion addon skeleton.
- Keep `ofxGgmlAgentsPlannerExample` as the first root-level smoke example with
  copyable planning handoff records.
- Keep `ofxGgmlCore` as the only required library dependency; combined examples may depend on `ofxGgmlRag` and `ofxImGui`.
- Add local validation and headless tests.
- Add independent addon version metadata and release-candidate docs.
- Document the `clone -> setup -> run` path from a new user's point of view.
- Add focused tests around request/result helpers.
- Maintain the model-backed allowlisted tool loop, cited local RAG tool, and
  selectable CPU/CUDA endpoint profiles in the existing planner example.

## Next Milestones

- Add another companion tool only after a concrete workflow needs it; keep the
  current combined example as the default orchestration surface.
