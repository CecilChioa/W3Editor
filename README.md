# W3Editor

Current mainline: `Rust + wgpu + winit + Slint` (no Bevy dependency).

## Workspace layout

- `native/core-map`: map domain, command bus, editor mode, property schema
- `native/render-wgpu`: rendering backend shell
- `native/app`: runtime entry and event loop
- `native/ui-slint`: UI crate

## Build

```bash
cd native
cargo check
```

## Run

```bash
cd native
cargo run -p app
```
