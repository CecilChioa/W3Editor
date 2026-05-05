# W3Editor Project Plan (Native Mainline)

## Goal

Build a standalone Warcraft III terrain editor that can create/open/save `.w3x/.w3m`, edit terrain and doodad/destructable placement, and remain stable under long editing sessions.

## Baseline

- Map size: `480 x 480`
- Object count target: `~50,000`
- Priority: performance + memory stability

## Architecture

```text
native/
  core-map/      (format/parser/domain/commands)
  render-wgpu/   (device/surface/pipeline/render loop pieces)
  app/           (winit loop + command dispatch + integration)
  ui-slint/      (tool panels, property editors, status surface)
```

## Technical Decisions

1. Renderer: `wgpu` (direct, no Bevy dependency).
2. Window/Input: `winit`.
3. UI: Slint for desktop tooling surfaces.
4. Editing model: command-driven pipeline for deterministic undo/redo and large-batch operations.
5. Property system: schema-driven by editor mode, reusable panel rendering.

## Current Status

- Native workspace migrated away from Bevy.
- `core-map` mode/command/property skeleton in place.
- `render-wgpu` + `app` minimal clear loop running path established.
- Legacy Tauri/Vite path removed from repository root.

## Next Milestone

Implement real map parsing in `core-map` and connect parsed map summary into `app` startup flow, then expose it in Slint UI.
