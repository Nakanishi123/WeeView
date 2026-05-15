## Docs map

- Behavior/specification entry point: `docs/spec.md`
- Input and sorting behavior: `docs/specs/input-and-sorting.md`
- Viewer behavior: `docs/specs/viewer-behavior.md`
- UI behavior: `docs/specs/ui-behavior.md`
- Settings/history behavior: `docs/specs/settings-history.md`
- Architecture and ownership: `docs/architecture.md`
- Implementation plan: `docs/implementation-plan.md`
- Decisions and deferred work: `docs/decisions.md`

## Commands

- Build: `mise run build`
- Format: `mise run format`

Run these from the repository root.

## Rules

- Read relevant docs before non-trivial changes.
- Do not change documented behavior unless explicitly asked.
- If docs and code conflict, report it before choosing a direction.
- Update docs and tests when behavior changes.
- Keep build-fix changes minimal; do not disable tests or remove targets to pass builds.
