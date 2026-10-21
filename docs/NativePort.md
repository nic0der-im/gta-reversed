# Native (non-Windows) port

Status of the `NOTSA_NATIVE` build mode (`GTASA_NATIVE` CMake/Conan option): a macOS arm64
executable that does not need `gta_sa.exe` at runtime. Build with
`python3 setup.py --build -p macos-arm64.txt`.

Everything platform specific lives behind `#ifdef NOTSA_NATIVE` or in
`source/app/platform/native/`. The Windows `.asi` build is unaffected.

## Where the native binary stops today

`gta_reversed --game-dir <dir>` creates its SDL3 window, reads `models/GTA3.IMG` and then aborts
with `Unreversed exe function 0x745510 called` (`psGetMemoryFunctions`, i.e. RenderWare's memory
functions have no in-repo definition yet).

| Address | Symbol | Hit by |
| --- | --- | --- |
| 0x745510 | `psGetMemoryFunctions` | native startup |

Append a row for every address the native build aborts on. The column is the reason the native
build reached it, so it doubles as a work list for reversing.

## Renderer decision: librw

`spikes/librw/` is a standalone spike that settles whether librw can render San Andreas assets on
this Mac. It uses librw `18532c2e13efbc1aa43f0b00b4459831f9414de0`, `LIBRW_PLATFORM=GL3`,
`LIBRW_GL3_GFXLIB=SDL3`.

Observed on an Apple M5, macOS 27:

- `GL_VERSION 4.1 Metal - 91.7`, `GLSL 4.10`, `GL_EXT_texture_compression_s3tc` present.
- A DFF/TXD pair read straight out of the retail IMG renders correctly: `infernus.dff` (15
  atomics, 3072 tris, 62 materials) with `infernus.txd` + `vehicle.txd`; all 22 D3D9 textures
  convert to GL3 with no failures. `male01.dff` renders skinned (32 bones) with its 128x256 DXT
  texture.
- Screenshots: `spikes/librw/screenshots/{triangle,car,ped}.png`. Full notes:
  `spikes/librw/FINDINGS.md`.

Decision: **GO** for librw as the renderer for every non-Windows target (GL3 today, GLES for
Android/WASM later). Conditions:

- Every texture read from a SA TXD must be passed through `Raster::convertTexToCurrentPlatform`
  by the caller; librw does not do it automatically.
- librw silently skips Rockstar-specific chunks (EnvMat, SpecMat, Collision, NodeName, 2dEffect,
  night vertex colours, pipeline id, breakable). Those plugins have to be written before the game
  can use the library, in the same way re3 does for GTA III/VC.
- SA's own pipelines (CustomBuilding, CustomBuildingDN, vehicle paint/env map, ped skin with
  specular) need GL3 pipeline implementations.
- librw's camera `rasterLock` omits `GL_PACK_ALIGNMENT = 1` (see
  `spikes/librw/screenshots/librw_readback_shear_625px.png`); carry a fix upstream or in our fork.
