# AGENTS.md

## Project Context

- Nearlighter is a learning-oriented CPU path tracer based on the Ray Tracing in One Weekend series.
- Current priority is project cleanup and optimization before adding larger rendering features.
- Code layout: public headers in `include/nearlighter/`, implementations in `src/`, and submodules in `thirdparty/`.
- Key docs: `README.md`, `scripts/README.md`, `tests/README.md`, and `docs/`.

## Documentation and Output Conventions

- Communicate with the user and write user-facing documentation in Chinese.
- Code, code comments, identifiers, commit messages, and agent-facing technical notes should use plain English.
- Keep explanations concrete and engineering-focused. Avoid decorative wording and emojis.
- Use standard GitHub-Flavored Markdown for user-facing responses, terminal-oriented Markdown output, and generated Markdown documents so they render correctly in common editors.
- Write inline mathematics as `$ ... $`; do not use `\( ... \)` delimiters.
- Write display mathematics on their own lines within `$$ ... $$`; do not use `\[ ... \]` delimiters.
- Keep each multiline display formula inside one pair of `$$` delimiters, and insert line breaks only where the mathematical structure requires them.
- Do not prefix display formulas with meaningless Markdown headings such as `##`.

## Engineering Principles

- Favor semantically coherent abstractions, clear responsibility boundaries, reusable modular APIs, and simple, efficient implementations. Avoid redundant layers and generalized infrastructure without a current requirement.
- Start from requirements and first principles. Clarify unclear goals; challenge inefficient proposals and suggest better alternatives before implementation.
- Default to discussion and alignment before writing non-trivial code.
- Preserve the current project style where practical, but improve clarity, correctness, and C++ hygiene when touching code.
- Keep changes scoped. Do not mix broad refactors with feature work unless the refactor is necessary for that feature.

## Planning Conventions

- Keep `ref/plans/` for agreed stage-level static plans. Once implementation begins, do not rewrite them merely to record progress.
- Include the stage background, objectives, scope and non-goals, current gaps, architecture and naming decisions, rationale and tradeoffs, phased work, dependencies, risks, tests, and acceptance criteria.
- Before implementation, read the static plan completely and use Plan mode for the fine-grained dynamic plan. Do not copy execution status back into the static plan.
- Store durable implementation status, deviations, and verification records under `ref/plans/status/`. Material design changes require user alignment and an explicit revision or addendum.

## Codebase Conventions

- Headers should include dependencies required by value members, bases, inline code, and public contracts. Forward declare only when a declaration, pointer, or reference is sufficient.
- Keep umbrella headers out of low-level module headers. `core.h` and `io.h` expose their modules; `nearlighter.h` exposes the complete SDK for external convenience.
- Keep meaningful parameter names in public declarations, including parameters unused by inline default implementations. Use C++17 `[[maybe_unused]]` when necessary; do not remove names merely to silence compiler warnings. Out-of-line definitions may omit genuinely unused names when the public declaration already documents them.
- Keep established technical acronyms uppercase in identifiers, such as `AABB`, `BVH`, `PDF`, `PPM`, and `RGB`.
- Prefer names that express a map's domain role. Use `<key>_to_<value>` when the lookup relationship needs clarification; keep natural registry or index names when context already makes the key clear.
- Keep third-party code in `thirdparty/` submodules and expose it through CMake targets.
- Prefer target-based CMake (`target_sources`, `target_include_directories`, `target_link_libraries`) over global include/link settings.
- Avoid global mutable state in new rendering code, especially for random generators and output/gamma configuration.

## Testing Conventions

- Test core behavior whose regressions can remain silent; skip trivial behavior and failures already obvious during normal execution.
- Keep tests small, deterministic, focused, and proportionate to the regression risk. Add coverage before changing performance-sensitive code such as BVH, sampling, or intersection routines.
- Document each test's purpose, coverage, and logic in `tests/README.md` in the same change.

## Commenting Conventions

### Principles

- Comment non-obvious rendering, geometry, math, ownership, numerical, coordinate-space, and error-handling logic generously.
- Explain intent, rationale, assumptions, constraints, and invariants; never restate code. Keep comments current.
- Read `ref/comment_style.md` for detailed guidance and examples when dealing comments.

### Comment Hierarchy

- Level 1 — file, module, or class sections
  + Use concise banners for recognizable responsibility groups; omit them in small single-purpose files.
  + C++: file or class sections with `//`; retain Doxygen on public declarations.
  + Python: module sections with `#`; use two blank lines above except at file start and one below.

  ```c++
  // ==================================================
  // Section Title
  //   Optional relationship or structure
  // ==================================================
  ```

  ```python
  # ==================================================
  # Section Title
  # ==================================================
  ```
- Level 2 — classes and functions
  + C++: concise Doxygen blocks (`/** ... */`) before declarations, never `///`; document useful contracts, invariants, ownership, and tags without duplicating them on `.cpp` definitions.
  + Python: concise `'''...'''` docstrings as the first statement; mention only useful contracts, units, failures, or side effects without repeating annotations or implementation.
- Level 3 — function phases
  + Upper group, optional for nested phases: C++ `/* ===== Short Title ===== */`; Python `# ===== Short Title =====`.
  + Common stage, preferred default: C++ `/* ----- Short Title ----- */`; Python `# ----- Short Title -----`.
  + Use short noun phrases and mark all sibling stages needed to reveal the structure. Do not use phase headings for individual objects or local operations.
- Level 4 — block descriptions
  + C++: ordinary `/* ... */`; Python: ordinary `# ...`.
  + Describe an object, relationship, or non-obvious logic within one stage when a local comment is too weak.
- Level 5 — local details
  + C++: `//`; Python: `#`.
  + Use for subordinate implementation notes; keep end-of-line comments short.

## Skill Proposals

- Notice recurring repository-specific workflows that may warrant a Codex skill, but do not create or substantially update one without explicit approval.
- Before requesting approval, explain its use cases, purpose, scope, triggers, layout, supporting files, and validation.
- After approval, follow the official Codex skill-creation workflow and place repository-specific skills under `.agents/skills/`, and validate the result.
- Keep one-off facts and general rules in documentation, tests, or `AGENTS.md` instead.

## Safety Notes

- Do not revert user changes unless explicitly requested.
- Generated build outputs and rendered images should stay out of source control.
- Before large refactors, first confirm the intended directory layout and migration order with the user.
