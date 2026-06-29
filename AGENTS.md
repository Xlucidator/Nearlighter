# AGENTS.md

## Project Context

- Nearlighter is a learning-oriented CPU path tracer based on the Ray Tracing in One Weekend series.
- Current priority is project cleanup and optimization before adding larger rendering features.
- Core code currently lives in `src/`; vendored third-party headers currently live in `include/`.
- Important docs: `README.md`, `doc/note.md`, and `doc/hit-calculation.md`.

## Collaboration

- Communicate with the user in Chinese.
- User-facing documentation should be written in Chinese.
- Code, code comments, identifiers, commit messages, and agent-facing technical notes should use plain English.
- Keep explanations concrete and engineering-focused. Avoid decorative wording and emojis.

## Engineering Principles

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
- Keep `common.h` as a curated core vocabulary only. Do not grow it into a cross-module include hub for shapes, materials, textures, BVH, PDFs, or scene code.
- Forward declare types only when a pointer/reference/declaration is enough and the complete type is not required. Include concrete headers in `.cpp` files for implementation details.
- Keep third-party code separate from project code. Prefer `thirdparty/` submodules plus CMake targets for external libraries.
- Prefer target-based CMake (`target_sources`, `target_include_directories`, `target_link_libraries`) over global include/link settings.
- Avoid global mutable state in new rendering code, especially for random generators and output/gamma configuration.
- Add benchmarks or small deterministic tests before changing performance-sensitive code such as BVH, sampling, or intersection routines.
- Add clear, structured comments for non-obvious rendering, geometry, math, ownership, and numerical assumptions. Prefer concise blocks that state intent, formula, assumptions, and invariants.

## Safety Notes

- Do not revert user changes unless explicitly requested.
- Generated build outputs and rendered images should stay out of source control.
- Before large refactors, first confirm the intended directory layout and migration order with the user.
