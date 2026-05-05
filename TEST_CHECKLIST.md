# W3Editor Test Checklist (Phase 0-1)

## Scope

This checklist validates current implemented features:

- Tauri frontend/backend connectivity
- Open map basic metadata
- Parse map W3E summary via .w3x/.w3m
- Parse W3E summary from direct file path
- Select map file dialog
- Recent map path persistence
- Recent map list quick-select
- Busy/disabled UI states during long operations
- Diagnostics report export
- Startup preflight checks

## Test Environment

1. Windows 10/11 x64
2. Node.js 18+
3. Rust stable toolchain
4. At least one valid Warcraft III map `.w3x` or `.w3m`

## Start App

1. Run `npm install` in `C:\war3\mytool\W3Editor`
2. Run `npm run tauri:dev`
3. Confirm app window opens

## Functional Tests

1. Click `Ping Rust`
Expected:
- status changes to `Backend connected`
- logs contain `[ok] w3editor-backend-ok`

2. Click `Browse`, select a valid `.w3x/.w3m`
Expected:
- map path input is updated
- logs contain `[select] <path>`

3. Click `Open Map`
Expected:
- status changes to `Map opened`
- logs contain `map.w3e`, `map.w3i`, `map.doo` lines
- `Map Info` panel shows name/author/recommended/tileset/size/doodads

4. Restart app
Expected:
- last opened map path is auto-restored in map path input

5. Open at least two different maps, then use `Recent Maps` dropdown
Expected:
- dropdown shows recent entries
- selecting an entry fills map path input

6. Click `Open Map` / `Export Diagnostics` / `Parse W3E Summary`
Expected:
- related button text switches to `Opening...` / `Exporting...` / `Parsing...`
- operation buttons become disabled during in-flight request

7. Click `Export Diagnostics`
Expected:
- status changes to `Diagnostics exported`
- logs contain `[diag] <output path>`
- output JSON file exists and contains app/status/map_path/map_info fields

8. Enter valid `war3map.w3e` file path in `W3E Path (debug)` and click `Parse W3E Summary`
Expected:
- status changes to `W3E parsed`
- logs contain `[w3e]` summary line

9. Run `npm run preflight`
Expected:
- output contains `[preflight] ok`

## Error Handling Tests

1. Leave map path empty and click `Open Map`
Expected:
- status `Map open error`
- logs contain `[err] map path is empty`

2. Enter non-existent map path and click `Open Map`
Expected:
- readable error with `map file not found`

3. Enter file with non-map extension and click `Open Map`
Expected:
- readable error with `unsupported map extension`

4. Enter a map missing internal files (`war3map.w3e` / `war3map.w3i` / `war3map.doo`)
Expected:
- readable error with missing internal file hint

## Exit Criteria For Next Phase

All above tests pass on at least one real map file.
