# librw macOS/arm64 + GTA San Andreas asset spike

Standalone spike (not part of the game build) proving that librw renders on this
Mac and can load and draw unmodified GTA:SA `gta3.img` DFF/TXD assets.

**Verdict: GO** for librw as the SA renderer (plan B3), with the conditions in
[Needed for SA](#needed-for-sa-and-risks).

## Environment

| Item | Value |
|---|---|
| Machine | Apple M5, macOS 27.2 (darwin/arm64) |
| Compiler | Apple clang (Xcode CLT, `/usr/bin/c++`), C++17 |
| CMake / Ninja | Homebrew, `-G Ninja`, Release |
| librw | commit `18532c2e13efbc1aa43f0b00b4459831f9414de0` (2026-08-26), fetched by `FetchContent` |
| librw platform | `LIBRW_PLATFORM=GL3`, `LIBRW_GL3_GFXLIB=SDL3` |
| Windowing | SDL3 3.4.18 (Homebrew `sdl3`), core-profile GL context created by librw |
| GL strings | `GL_VENDOR Apple`, `GL_RENDERER Apple M5`, `GL_VERSION 4.1 Metal - 91.7`, `GLSL 4.10`, 43 extensions, max texture 16384 |
| S3TC | `GL_EXT_texture_compression_s3tc` present → librw enables DXT (`gl3Caps.dxtSupported`) |

librw already supports SDL3 upstream (`LIBRW_GL3_GFXLIBS="GLFW" "SDL2" "SDL3"`).
**No patch to librw was needed.** librw's `startSDL3()` creates the window and GL
context itself (SDL_GL 3.3 core profile, falling back to 2.1/GLES), so the glue
we wrote is only: engine init/plugins/open/start, event loop, resize handling,
scene setup and framebuffer capture. `EngineOpenParams` carries `SDL_Window**`,
width, height and title.

## Build / run

```sh
brew install sdl3        # already installed on this machine
cmake -S spikes/librw -B spikes/librw/build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build spikes/librw/build -j 8

spikes/librw/build/librw_spike --stage=triangle --seconds=4 --frame=45
spikes/librw/build/librw_spike --stage=car      --seconds=5 --frame=75
spikes/librw/build/librw_spike --stage=ped      --seconds=5 --frame=60
# options: --game-dir=PATH (default $GTASA_DIR or ~/Games/GTASA), --out-dir=PATH,
#          --dff=NAME --txd=NAME, --no-spin, --librw-readback
```

`gta3.img` is parsed in memory (no extraction to disk); the game directory is
never copied into the repo. Each run exits by itself, prints GL/asset
diagnostics and writes one PNG read back from the GL back buffer.

## Stage 1 — window + librw + triangle

Works. librw `Engine::init()` → plugins → `open()` → `start()` creates a
1280×800 SDL3 window; the immediate-mode path
(`rw::im3d::Transform/RenderPrimitive/End`, `gl3::Im3DVertex`) draws a
vertex-coloured triangle over a cleared camera. 59 fps with vsync.

Evidence: `screenshots/triangle.png` (625×998 window after the WM resized it;
red/green/blue gouraud triangle, ~500 distinct colours, mean rgb 35,37,48).
Log: `logs/triangle.log`.

Note: the environment's window manager resizes new windows (1280×800 was
reported at creation, then tiled to e.g. 625×998). The spike handles
`SDL_EVENT_WINDOW_RESIZED` / pixel-size changes by recreating the camera rasters
and re-applying the FOV, so captures always match the real window.

`screencapture -x` fails in this session with
`could not create image from display` (Screen Recording permission is not
granted to the shell/agent context; `-l <windowid>` needs the same permission),
so all visual evidence is a `glReadPixels` readback of the presented back buffer,
which is the exact frame librw rendered.

## Stage 2 — SA assets

### Car (`infernus.dff` + `vehicle.txd` + `infernus.txd`)

* IMG v2: header `VER2`, 16297 entries; entries are
  `u32 offsetSectors, u32 sizeSectors, char name[24]` (`offset*2048`,
  `size*2048`; verified against the RW chunk header: `infernus.dff` = 105
  sectors, clump chunk `0x10 size 213850 version 0x1803FFFF`).
* `TexDictionary::streamRead` works on the SA PC TXDs. Textures are D3D9 native
  textures (platform id 9). **librw does not convert them automatically**:
  `Texture::streamReadNative` dispatches to `d3d9::readNativeTexture`, which
  returns a `PLATFORM_D3D9` raster. The caller must run
  `tex->raster = Raster::convertTexToCurrentPlatform(tex->raster)` (this is the
  `d3d_to_gl3` path for DXT1/3/5 plus an Image fallback for uncompressed).
  Done here: 22 textures (19 from `generic/vehicle.txd`, 3 from
  `infernus.txd`), **22 converted, 0 failed**; 3 are DXT (depth 16), the rest
  32-bit; sample DXT ones: `infernus92wheel32`, `infernus92interior128`,
  `infernus92handle32`.
* DFF: 15 atomics, 3573 vertices, 3072 triangles, 62 textured materials, all
  texture lookups resolved (0 without raster), 18 materials carry MatFX env-map
  data (`MatFX::getEffects == 0x2`), which librw reads and renders.
* Rendered spinning with the default GL3 pipeline: body, windows, interior,
  wheels, lights and the grunge/dirt texture are visible.

Evidence: `screenshots/car.png`; log: `logs/car.log`.

The body's pale panels are the DFF's white paint material and the green parts
are the DFF's grunge material colour (60,255,0). In-game the vehicle colour is
assigned per instance by the game, not by the DFF, so this is expected for a raw
DFF viewer. SA's own env-map/specular material chunks are dropped (see below),
so reflections/specular dots are missing.

### Ped (`male01.dff` + `male01.txd`)

* 1 atomic, 1166 vertices, 1209 triangles, **skin present (32 bones)**, HAnim
  chunks present (32 × `0x11E`), texture `male01` 128×256 DXT converted OK.
* The skinned pipeline works. The DFF carries no atomic-level hierarchy link,
  so librw's skin renderer uses identity bone matrices — which is exactly the
  bind pose. Result: fully textured ped (face, denim jacket, hoodie, trousers,
  sneakers) standing in bind pose.
* SA ped DFFs are authored **Y-up** (head towards +Y). The spike rotates the
  clump root +90° about X before framing; the game must do the equivalent
  (its ped rendering code already accounts for this).

Evidence: `screenshots/ped.png`; log: `logs/ped.log`.

### librw bug found: camera readback shears for widths not divisible by 4

`src/gl/gl3raster.cpp`, `rasterLock`, `case Raster::CAMERA`: `glReadPixels` is
called without `glPixelStorei(GL_PACK_ALIGNMENT, 1)`, while the code assumes
`stride = width*3`. At width 625 (625·3 = 1875, not a multiple of 4) the driver
pads every row and the decoded image shifts one byte every four rows.
Reproduced with `--librw-readback` at 625×998:
`screenshots/librw_readback_shear_625px.png`. The spike's own capture path sets
`GL_PACK_ALIGNMENT=1` (and is otherwise identical), producing correct
`screenshots/ped.png`. Suggested upstream fix: set the pack alignment in the
CAMERA lock (or read through the FBO texture). This only affects screenshots,
not rendering.

## What SA DFFs contain and what librw reads

Chunk IDs seen in `infernus.dff` / `male01.dff` (RW 3.6, `0x1803FFFF`):

| Chunk | librw | Seen in |
|---|---|---|
| `0x120` MatFX | read (`registerMatFXPlugin`) | infernus: 18 env-map materials |
| `0x116` Skin | read (`registerSkinPlugin`) | male01 (25412-byte chunk at unaligned file offset 65906) |
| `0x11E` HAnim | read (`registerHAnimPlugin`) | male01: 32 HAnim chunks (hierarchy + node data) |
| `0x50E` Mesh / `0x11F` userdata / rights | plugins available; not observed in these files (librw builds meshes from triangles when absent) | — |
| `0x253F2F6` SpecMat | **skipped** | infernus: payload `float 0.3` + `"vehiclespecdot64"` |
| `0x253F2FC` EnvMat | **skipped** | validated extension @15074 (24-byte chunk) |
| `0x253F2FA` Collision | **skipped** | validated extension @209310 (4528-byte chunk) |
| `0x253F2FE` NodeName | **skipped** | validated extension @3283 in infernus (used by game logic, e.g. wheel/component names) |
| `0x253F2F8` 2dEffect, `0x253F2F9` extra (night) vertex colours, `0x253F2F3` pipeline id, `0x253F2FD` breakable | **skipped** | not needed to draw, needed by the game |

`PluginList::streamRead` (`src/plg.cpp`) silently skips unknown extension
chunks, which is why the models load despite the missing plugins. librw has **no**
`registerNodeNamePlugin`, extra-vertex-colour, env/spec material, 2dEffect,
collision or breakable plugins.

## Needed for SA (and risks)

1. **Rockstar plugins must be written** (or differenced from re3/librwgta, whose
   README declares its code GPL — not copyable without a licence decision):
   NodeName (0x253F2FE), EnvMat (0x253F2FC), SpecMat (0x253F2F6), extra night
   vertex colours (0x253F2F9), 2dEffect (0x253F2F8), collision (0x253F2FA),
   breakable (0x253F2FD), pipeline id (0x253F2F3), plus SA breakable/extra
   normals if map assets need them. Formats can be recovered from the chunks and
   from the game binary.
2. **TXD conversion glue**: wrap `TexDictionary::streamRead` with
   `Raster::convertTexToCurrentPlatform` for every texture (or patch librw's
   native-texture read to convert for the current platform). PC files are D3D9;
   librw also has PS2/Xbox readers but those paths were not exercised.
3. **Custom GL3 pipelines**: the default pipeline draws the tested car and ped,
   but SA's CustomBuilding/CustomBuildingDN, vehicle env-map/dual-texture and
   specular effects (and per-vehicle material colours) need own GLSL 330
   pipelines, mirroring the D3D9 ones reversed from the exe. The MatFX env-map
   material path already exists in librw (`gl3matfx.cpp`) and worked.
4. **Presentation**: on GL3 a `Raster::CAMERA` is the window back buffer
   (`rasterCreateCamera` sets `fbo = 0`), and `showRaster` only swaps. That maps
   directly onto `psCameraShowRaster`; `psGrabScreen` should use a readback with
   `GL_PACK_ALIGNMENT=1` (see bug above).
   Note for B1.5/B3: librw's SDL3 device creates the window **and** GL context
   itself in `Engine::start()` (`startSDL3`) and writes it back through
   `EngineOpenParams::window`. `NativeMain.cpp` must therefore not pre-create a
   window with `SDL_CreateWindow` (or librw must be changed to accept one); the
   game can take the pointer from the out-param and store it in
   `psGlobalType`.
5. **Skins**: bind pose works with identity bone matrices; animation needs the
   game to set the HAnim hierarchy (`Skin::setHierarchy`) from `HAnimData`,
   which the DFF hierarchy already provides.
6. **Y-up ped DFFs** need the root rotation described above.

## Risks

* SA-specific pipelines are the largest unknown: building renders nothing
  custom yet, and the vehicle look (env map, specular dots, per-instance paint)
  currently comes only from raw DFF material colours.
* Missing NodeName/collision/2dEffect plugins are load-bearing for gameplay
  (wheels, doors, map collision, light coronas), but they are data plugins, not
  renderer blockers.
* librw assumes D3D-style texture platform chunks; every TXD must be converted
  explicitly (easy to forget, yields D3D9 rasters bound to GL).
* Upstream librw is a moving target; pin the commit (done here).
* `screencapture` is unusable in this environment; rely on in-app readback for
  visual regression.

## Artifacts

* `src/img.{h,cpp}` — IMG v2 reader (directory in memory, on-demand reads).
* `src/main.cpp` — SDL3 glue, librw init, TXD/DFF loading, camera/world, triangle
  and clump stages, GL readback, resize handling, diagnostics.
* `screenshots/triangle.png`, `screenshots/car.png`, `screenshots/ped.png`,
  `screenshots/librw_readback_shear_625px.png`.
* `logs/triangle.log`, `logs/car.log`, `logs/ped.log` — full runs, including GL
  strings and per-texture/per-material diagnostics.
