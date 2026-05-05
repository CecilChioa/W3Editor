# W3Editor Implementation Steps (Native)

## Phase 1 (done)

1. Native workspace established (`core-map`, `render-wgpu`, `app`, `ui-slint`).
2. Mode-based editor structure (`Terrain`, `Doodad`, `Destructable`, `Region`).
3. Unified command bus scaffold (`EditorCommand`, `CommandBus`).
4. Extensible property schema scaffold (`schema_for_mode`).
5. Legacy Tauri/Vite code and dependencies removed.
6. Bevy dependency removed; runtime now uses `wgpu + winit`.

## Phase 2 (next)

1. Migrate `w3e/w3i/doo/mpq` parsing into `core-map`.
2. Replace `open_map_basic_info` placeholder with real parsing.
3. Add `MapDocument` and stable error model.

## Phase 3

1. Terrain mesh rendering in `render-wgpu`.
2. Doodad placeholder instancing and basic culling.
3. Camera + ray picking in `app`.

## Phase 4

1. Selection/move/rotate/delete via command pipeline.
2. Undo/Redo.
3. Save back to `.w3x/.w3m`.

## Phase 5

1. Slint editor panels bound to command bus.
2. Large map performance pass (480x480, 50k objects).
3. Long-run memory stability tests.
