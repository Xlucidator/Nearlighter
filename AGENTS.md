# AGENTS.md

## Project Context

- Nearlighter is a learning-oriented CPU path tracer based on the Ray Tracing in One Weekend series.
- Current priority is project cleanup and optimization before adding larger rendering features.
- Public headers live in `include/nearlighter/`, implementations live in `src/`, and third-party submodules live in `thirdparty/`.
- Important docs: `README.md`, `doc/note.md`, `doc/hit-calculation.md`, and `doc/tests-note.md`.

## Collaboration

- Communicate with the user in Chinese.
- User-facing documentation should be written in Chinese.
- Code, code comments, identifiers, commit messages, and agent-facing technical notes should use plain English.
- Keep explanations concrete and engineering-focused. Avoid decorative wording and emojis.

## Engineering Principles

- Favor semantically coherent abstractions, clear responsibility boundaries, reusable modular APIs, and simple, efficient implementations. Avoid redundant layers and generalized infrastructure without a current requirement.
- Apply first-principles thinking. Do not assume the author always knows exactly what they want or the best way to achieve it.
- Start from the raw requirements and the core problem.
  + If the goal or motivation is unclear, stop and align before implementation.
  + If the proposed path is not optimal or performant, say so and suggest a better approach before implementation.
- Default to discussion and alignment before writing non-trivial code.
- Preserve the current project style where practical, but improve clarity, correctness, and C++ hygiene when touching code.
- Keep changes scoped. Do not mix broad refactors with feature work unless the refactor is necessary for that feature.

## Codebase Conventions

- Prefer explicit local dependencies. A header may include the headers needed for its own value members, base classes, inline code, and public API contracts.
- Umbrella headers are acceptable when they are intentional, stable, and convenience-oriented. Keep them thin, documented, and out of low-level module headers.
- Keep `nearlighter.h` as a curated core vocabulary only. Do not grow it into a cross-module include hub for shapes, materials, textures, BVH, PDFs, or scene code.
- Forward declare types only when a pointer/reference/declaration is enough and the complete type is not required. Include concrete headers in `.cpp` files for implementation details.
- Keep meaningful parameter names in public declarations, including parameters unused by inline default implementations. Use C++17 `[[maybe_unused]]` when necessary; do not remove names merely to silence compiler warnings. Out-of-line definitions may omit genuinely unused names when the public declaration already documents them.
- Keep third-party code separate from project code. Prefer `thirdparty/` submodules plus CMake targets for external libraries.
- Prefer target-based CMake (`target_sources`, `target_include_directories`, `target_link_libraries`) over global include/link settings.
- Avoid global mutable state in new rendering code, especially for random generators and output/gamma configuration.

## Testing Conventions

- Add tests for core behavior whose regressions can remain silent during normal use. Do not test trivial behavior or failures that normal execution already exposes clearly.
- Keep tests small, deterministic, focused, and proportionate to the regression risk. Add coverage before changing performance-sensitive code such as BVH, sampling, or intersection routines.
- Document every test topic in `doc/tests-note.md`, including its purpose, coverage, and implementation logic. Update the document in the same change that adds or materially changes a test.

## Commenting Conventions

### General Principles

- Write comments proactively and generously where they improve readability, especially for non-obvious rendering, geometry, math, ownership, numerical, coordinate-space, and error-handling logic.
- Explain intent, rationale, assumptions, constraints, and invariants rather than restating the code.
- Update or remove stale comments when the implementation changes.

### Comment Styles

- Use Doxygen block comments (`/** ... */`) for classes and functions; do not use `///`. Place public API documentation before its declaration, describe the contract where relevant, and use tags such as `@param`, `@return`, and `@throws` only when helpful. Do not duplicate it on the `.cpp` definition.
- Use ordinary block comments (`/* ... */`) inside function implementations to introduce major logical sections, algorithm stages, or process blocks.
- Use line comments (`//`) for subordinate steps, specific implementation details, or short end-of-line clarification. Put longer explanations before the relevant code.

## Skill Proposals

- Notice recurring repository-specific workflows or non-obvious knowledge that may warrant a Codex skill, but do not create or substantially update one without explicit user approval.
- Before requesting approval, explain:
  + the recurring problem or concrete use cases;
  + the skill's purpose, scope, and expected benefit;
  + trigger scope: when it should and should not trigger;
  + the proposed name, location, instructions, references, scripts, and validation method.
- After approval, follow the official Codex skill-creation workflow and place repository-specific skills under `.agents/skills/`, and validate the result.
- Keep one-off facts and general rules in documentation, tests, or `AGENTS.md` instead.

## Safety Notes

- Do not revert user changes unless explicitly requested.
- Generated build outputs and rendered images should stay out of source control.
- Before large refactors, first confirm the intended directory layout and migration order with the user.
