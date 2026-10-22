# Native (non-Windows) port

Status of the `NOTSA_NATIVE` build mode (`GTASA_NATIVE` CMake/Conan option): a macOS arm64
executable that does not need `gta_sa.exe` at runtime. Build with
`python3 setup.py --build -p macos-arm64.txt`.

Everything platform specific lives behind `#ifdef NOTSA_NATIVE` or in
`source/app/platform/native/`. The Windows `.asi` build is unaffected.

## Where the native binary stops today

`gta_reversed --game-dir <dir>` creates its SDL3 window through librw (`RwEngineStart`), reads
`models/GTA3.IMG`, initialises the RenderWare engine, attaches the game's RenderWare plugins,
builds the camera/world, shows the EAX splash, plays the intro videos through the native video
player, loads the text GXT and `DATA/HANDLING.CFG` (259 handlings for 210 vehicles) and starts
the OpenAL backend.

`CGame::InitialiseEssentialsAfterRW` now succeeds. `CCarFXRenderer::Initialise` and
`CCustomBuildingRenderer::Initialise` no longer create RenderWare pipelines: the four GL3 San
Andreas pipelines (`Building`, `BuildingDayNight`, `Vehicle`, `Ped`) are built once at the end of
`RwEngineStart` (`notsa::native::sapipelines::RegisterSAPipelines`), and the `CCustom*Pipeline`
classes take their handles from `sapipelines::GetPipeline` (native branch of `CreatePipe`).
`RenderWare/librw/RxPipeline.cpp` implements the generic `Rx*` pipeline API on top of them: a
real `RxPipeline`/`RxPipelineNode` pair with one harmless generic node, the San Andreas node
definitions (`RxNodeDefinitionGetD3D9*AllInOne`), lock/unlock/fragment/find, and a
`RxPipelineDestroy` that never frees the shared San Andreas handles. `RpAtomicInstance`,
`AtomicDefaultRenderCallBack` and `RpGeometryDestroy` forward to
`InstanceAtomic`/`RenderAtomic`/`ForgetGeometry`.

The native build then enters `MainLoop` and draws the frontend. The two game-side calls that
aborted there were font draws, now native log-once no-ops: `CFont::RenderFontBuffer` (queues
glyphs for Im2D) and `CFont::ProcessCurrentString` (text layout + that queue). Both only feed
`FontRenderStateBuf`, which `CFont::InitPerFrame` resets every frame; the Im2D backend itself is
B3.

The next boundary was **audio, not rendering**, and it was not an audio bug: the main loop runs
`FrontendIdle` → `CAudioEngine::Service` → `CAESoundManager::Service` → `CAESound::UpdateParameters`,
which SIGSEGV'd (`EXC_BAD_ACCESS` at 0x0) on the `m_AudioEntity->UpdateParameters(this, curPlayPos)`
virtual call (`ldr x8, [x8]` with `x8 == 0`): the audio entity object's vtable pointer was null.

`CAudioEngine` lives at 0xB6BC90 through a `StaticRef`, so natively it is zero-filled `ExeGlobals`
storage. Its constructor (`0x84D5D0: mov ecx, 0xB6BC90 ; call 0x507670`) was one of the constructor
rows `ApplyExeData()` leaves unapplied, so `m_FrontendAE`, `m_ScriptAE`, `m_CollisionAE`,
`m_PedlessSpeechAE` and `m_DoorAE` were never constructed. Their only initial state is the vtable,
so every virtual call they receive jumped through address 0. `ApplyManualExeDataEntry` now runs
`::new (&AudioEngine) CAudioEngine{}` for that row (`ExeData.manual.cpp`).

`CAESound::UpdateParameters` also gets a portable counterpart of the retail SEH guard: retail wraps
`m_PhysicalEntity->GetPosition()` in `__try/__except` because the entity can be stale. The native
branch validates the pointer with `IsEntityPointerValid` (the repo's pool-based dangling-pointer
check) and performs the handler's `StopSound()` when it fails (`AESound.cpp:259-269`).

The frontend loop now runs. `Rw: stub RpWorldAddCamera` / `RpWorldAddLight` /
`RtAnimRegisterInterpolationScheme` still log; they are no-ops and do not gate startup. The
frontend draws nothing yet (Im2D is B3), and OpenAL rejects the soft loop points of streaming
sounds (`AL_LOOP_POINTS_SOFT`, `OALSource.cpp:128`), which is a B4 item and not fatal.

The next boundary is therefore **rendering (B3)**, not a crash: with the loop stable, what is left
before the menu is visible is the Im2D/text backend (`CFont::RenderFontBuffer`,
`CFont::ProcessCurrentString`, the two log-once no-ops above) and the GL pipelines for the 3D
scene. On the B2 side, 96 constructor rows remain unapplied; they are objects nobody has called a
virtual on yet, so they are only noticed when a subsystem starts using one.

Observed run (`timeout 190`, 2026-10-21):

```
[info]    Rw: engine started (640x448 window)
[warning] Rw: stub RpWorldAddCamera
[warning] Rw: stub RpWorldAddLight
[debug]   CTxdStore::Create(7)
[debug]   Video: playing `movies/Logo.mpg` (640x480, 29.97 fps, 15.35s)
[debug]   Video: playing `movies/GTAtitles.mpg` (640x480, 29.97 fps, 88.72s)
[debug]   Loading 'AMERICAN.GXT' version=04 (8-bit ASCII)
[debug]   Successfully loaded 259x handlings for 210x vehicles!
[info]    OpenAL: OpenAL Soft (OpenAL Soft), 1.1 ALSOFT 1.24.3
[debug]   Initialised SoundManager
[warning] Native: CFont::RenderFontBuffer is a no-op until the Im2D backend (plan B3)
[warning] Native: CFont::ProcessCurrentString is a no-op until the Im2D backend (plan B3)
… frontend loop for the remaining ~74 s …
EXIT=124 (killed by `timeout`, no crash)
```

| Address | Symbol | Hit by |
| --- | --- | --- |
| 0x5617C0 | `GetMillisecondTime` | `CTimer::Initialise` (now `RsTimer()` natively, `Timer.cpp`) |
| 0x745510 | `psGetMemoryFunctions` | **no longer reached** (`app/platform/native/NativePs.cpp`) |
| 0x55D0E0 | `SurfaceInfos_c::LoadAdhesiveLimits` | **no longer reached** (reversed) |
| 0x719840 | `CFont::RenderFontBuffer` | copyright/loading screen draw; native log-once no-op (B3 Im2D) |
| 0x71A220 | `CFont::ProcessCurrentString` | menu text layout+draw; native log-once no-op (B3 Im2D) |

Append a row for every address the native build aborts on. The column is the reason the native
build reached it, so it doubles as a work list for reversing.

Checkpoint `762965f8` (wave 1) moved past the frontend: with the DFF and raster heap bugs fixed
(see *Porting hazards*) the game loads the world and reaches `MainLoop`. The narrative above
describes the 2026-10-21 state and is kept as the record of why the frontend no-ops exist.

## Exe-resident state (B2)

`tools/native/staticref_report.py` parses every `StaticRef`/`ScopedStaticRef` site in
`source/` (2373 sites, 2358 distinct addresses) and classifies each address against the sections
of the retail executable (`GTASA_EXE`). Output: `.local/native/staticrefs.csv`.

Current classification against the compact exe (`gta_sa.exe`, 5190656 bytes, image base
0x400000, image ends at 0x8B0904):

| class | sites | what it means |
| --- | --- | --- |
| `runtime` | 2316 | address is beyond the bytes the executable's sections carry on disk (0x8B0000-0xC9E000, the `.data` virtual tail): a region the retail exe allocates at startup. Storage is zeroed; values come from the exe's static initializers and from `CGame::Initialise*`. |
| `file` | 53 | inside a file-backed range (the initializer table `0x8A4000..0x8A5A10` and the constant pool around it, or `.data` raw up to 0x8E4000). Some of these are runtime-written globals (e.g. the cheat tables), so file bytes are not authoritative. |
| `zero` | 5 | in `.data` past its raw size (the exe's BSS). |

So B2 is not "copy the exe's data section"; it is "reimplement the exe's static initializers".
`tools/native/gen_exedata.py` decodes the 1667-entry `__xi_a..__xi_z` table and emits
`source/app/platform/native/ExeData.generated.{h,cpp}`, replayed at startup by
`notsa::native::ApplyExeData()` (writes go through `ExeGlobals`). Current split: 1489 float
stores, 10 byte stores, 9 fill-patterns, 5 zero-fills, 81 object constructors, 19 vector
constructors, 54 skips, 4 unhandled (documented in the file). Object constructors that need our
own C++ types live in `ExeData.manual.cpp`; the rest are listed as not-applied.

## Known 64-bit layout issues

`StaticRef` storage is per address, so two typed sites claiming the same address with different
sizes is a real divergence once pointers widen. A typed size that merely differs from an
initializer row's byte count is *not* one: the row is expressed in the executable's 32-bit element
layout, and a row that clears an object only covers part of it. `ExeGlobals::Get` therefore logs
`StaticRef ... requested with sizes ...` only when both requests are typed; initializer writes are
silent and grow the allocation when needed.

Rows whose element layout changed are not replayed byte for byte. `ExeGlobals::Write`/`ZeroFill`
hand them to `ApplyManualExeDataPattern`/`ApplyManualExeDataZeroFill` (`ExeData.manual.cpp`), which
apply them element by element through the typed container:

| address | executable row | native element | applied as |
| --- | --- | --- | --- |
| 0xB992B8 | `CWorld::ms_aRepeatSectors`, 256 x 0xC (ctor 0x563190) | 0x18 (`CRepeatSector`, three list heads) | every element cleared |
| 0xBC1C78 | `PlayerClothes`, cleared as 0xBC1C78: 40 bytes + 0xBC1CA0: 72 bytes | 0x78 (`CPedClothesDesc`) | whole object cleared |
| 0xC091F0 | `CGangs::Gang`, 10 x 0x10 (ctor 0x5DE520), `m_nPedModelOverride` = -1 | 0x10 (`CGangInfo`) | `Gang[i].m_nPedModelOverride = -1` |
| 0xC0B1E8 | `CPedScriptedTaskRecord::ms_scriptedTasks`, 128 x 0x14 (ctor 0x608330), `m_opcode` = -1 at +0x0 and `m_status` = FLUSHED at +0x10 | 0x28 (`CPedScriptedTaskRecordData`) | both members set, per element |

Adding a row is a table entry in `ExeData.manual.cpp`; the matcher is address based, so it needs
the executable address, the executable stride, the element count and the member mapping. Rows
whose element size did not change (the `CNodeAddress` arrays, for example) and rows no typed
reference reaches are still replayed as bytes.

What remains a genuine problem is `ReportOverlaps()`' list: two *typed* `StaticRef`s whose
allocations would overlap in the executable's address space because one native type is wider than
its executable counterpart. The largest case is `CWorld::ms_aRepeatSectors` (6144 bytes starting
at 0xB992B8), whose native extent covers the executable address of `CWorld::ms_aLodPtrLists`
(0xB99EB8, immediately after the repeat sectors' 256 x 0xC bytes). They are separate allocations
with separate pointers, so the game runs, but the addresses cannot be treated as one contiguous
global until the list storage is unified.

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

## Renderer (librw) layer: fakerw

`source/CMakeLists.txt` fetches librw with `FetchContent` (pinned to the commit above,
`LIBRW_PLATFORM=GL3`, `LIBRW_GL3_GFXLIB=SDL3`, fetched into `build/<config>/_deps`) and links
`librw::librw`. The native build excludes `source/game_sa/RenderWare/rw/*.cpp` (the retail thunks)
and `RenderWare/D3D*.cpp`; the Windows `.asi` keeps compiling them unchanged.

The same C API is reimplemented as a thin "fakerw" layer in
`source/game_sa/RenderWare/librw/`:

| file | contents |
| --- | --- |
| `RwFakerw.{h,cpp}` | `RwGlobals`/`RwEngineInstance` state (the plugin-sdk headers dereference it through `RWSRCGLOBAL`), `RwEngineInit/Open/Start/Stop/Close/Term`, video modes, `RwOsGetMemoryInterface`, the plugin-data registry, the `Rw: stub <name>` logger |
| `RwStream.cpp` | streams over stdio (memory, file, filename, custom) and chunk lookup |
| `RwCamera.cpp` | cameras and rasters; `RwCameraBeginUpdate/EndUpdate/Clear/ShowRaster` drive a librw camera and the GL window; **this is the camera present path** |
| `RwFrame.cpp` | frame hierarchy, `_rwObjectHasFrame*`, `RwFrameUpdateObjects`/`GetLTM` |
| `RwMatrix.cpp` | matrix/vector math |
| `RwImage.cpp`, `RwTexture.cpp` | images, textures, dictionaries |
| `RwWorld.cpp` | world/atomic/clump/geometry/light/material containers and Rw plugin attach |
| `RwDff.cpp` | binary DFF reader (clump, frame list, geometry, material, atomic, texture) and the material-list helpers; see *DFF streams and plugin data* |
| `RwD3D9.cpp` | `RwD3D9*` and `D3DResourceSystem` no-ops (GL3 replaces them in B3) |
| `RwStubs.generated.cpp` | generated by `tools/native/gen_rw_stubs.py`: one logged stub per thunk not implemented by hand |

Design notes:

- The game's structs stay authoritative, because game code reads their fields directly
  (`camera->farPlane`, `raster->width`, `frame->modelling`, ...). librw objects (camera, rasters,
  world) live next to them in side tables; `RwCameraBeginUpdate` copies the game state into the
  librw camera right before presenting.
- `RwEngineInit` installs the game's callbacks in `RwGlobals::memoryFuncs` and in librw's
  `MemoryFunctions`. The callbacks come from `notsa::native::GetRwMemoryFunctions()`
  (`app/platform/native/NativePs.cpp`), the native counterpart of the retail `0x745510`.
- `RwEngineStart` lets librw create the SDL3 window and the GL context and publishes it through
  `psGlobalType` (`PSGLOBAL(sdlWindow)`); `NativeMain` no longer creates a window.
- Objects with plugin data are allocated with trailing space (`PluginRegistry`), so
  `RWPLUGINOFFSET` offsets returned by `RwFrameRegisterPlugin`, `RpAtomicRegisterPlugin`, ... are
  valid and the constructors/destructors/copies run.
- Still stubbed (the generated file is authoritative: 541 entry points, one `Rw: stub <name>` log
  each): the stream *writers* and `*StreamGetSize` (`RpClumpStreamWrite`, `RpAtomicStreamWrite`,
  ...), the D3D9-only helpers (`RpD3D9*`, `_rpD3D9*`) and `RpBuildMesh*`. Hand-written stubs such
  as `RpClumpRender` (`RwWorld.cpp:503`) and the `RwIm3D*` pipelines stay until the GL3 draw path
  replaces them.
- `tools/native/gen_rw_stubs.py` re-derives the stub signatures from the retail thunks
  (so they always match the game headers) and skips every function defined in `librw/`;
  `--check` fails when the checked-in file is stale, like the other `tools/native/` generators.

Windows is unaffected: the layer is behind `#ifdef NOTSA_NATIVE` / `if(GTASA_NATIVE)`.

## Porting hazards

These failures all compile and link cleanly; each one only surfaces as a crash (or a corrupted
heap) somewhere unrelated to the bug that caused it.

### Heap integrity and raster formats

`RwRasterCreate` (`source/game_sa/RenderWare/librw/RwCamera.cpp:319`) translates
`rwRASTERFORMAT888` to `rwRASTERFORMAT8888` before calling `rw::Raster::create`.

Retail's D3D9 driver creates an 888 raster as a 32-bit X8R8G8B8 surface, and the game locks those
rasters and writes them as `RwRGBA` (4 bytes per texel). librw's `C888` is a packed 24-bit RGB8
format (3 bytes per texel), so forwarding the flag allocates a pixel buffer 25% too small and
every locked write runs past its end.

The overrun is silent until the allocator notices, which natively happens in `CMemoryMgr::Malloc`
(`source/game_sa/MemoryMgr.cpp:381`: `::malloc` plus the native block registry), so the abort
points at an arbitrary later allocation rather than at the writer. It showed up as `SIGTRAP`
(`malloc_error_break`) from `CMemoryMgr::Malloc` in unrelated subsystems. `NativeCrash.cpp:76`
now installs a handler for `SIGTRAP` too; before that, those aborts produced no backtrace.

The same trap is any locked write that assumes a 4-byte pitch:
`CCarFXRenderer::InitialiseDirtTexture` (`source/game_sa/Fx/CarFXRenderer.cpp`) takes the pitch
and the pixel size from `RwRasterGetStride`/`RwRasterGetDepth`, because `vehiclegrunge256` loads
as a 24-bit texture. When mapping another format, compare the retail surface's bytes per texel
with librw's, not the constant names.

Never `realloc` a buffer that game objects point into. `FxMemoryPool_c::Optimise`
(`source/game_sa/Fx/FxMemoryPool.cpp:47`) is a no-op for that reason: native `realloc` may move
the block, and every `FxSystemBP_c`/`FxPrimBP_c`/`FxEmitterBP_c` lives inside the pool and is
referenced from `FxManager_c::m_FxSystemBPs` and from each other, so a relocation leaves those
lists pointing at freed memory (which then surfaced as corruption inside `CMemoryMgr::Malloc`
during streaming).

### Material lists: deinitialize does not destroy

`_rpMaterialListDeinitialize` (retail `0x74E150`) releases the material array only; the list does
not own the materials it points at. `_rpMaterialListDestroy` (static, same translation unit)
destroys each `RpMaterial` and then calls the former. Both live in
`source/game_sa/RenderWare/librw/RwDff.cpp:877-905`.

- `RpGeometryDestroy` and a failed `_rpMaterialListStreamRead` use `_rpMaterialListDestroy`: there
  the list does own its materials.
- `CVehicleModelInfo::ReduceMaterialsInVehicle`
  (`source/game_sa/Models/VehicleModelInfo.cpp:506`) fills a scratch list with the clump's *live*
  materials and must use `_rpMaterialListDeinitialize`; destroying them leaves every geometry of
  that clump with dangling material pointers, right before the scratch pad is released.

### DFF streams and plugin data

The DFF reader is `source/game_sa/RenderWare/librw/RwDff.cpp`. Plugin data is dispatched through
`PluginRegistry::StreamReadExtension` (`source/game_sa/RenderWare/librw/RwFakerw.cpp:227`):

- The `rwID_EXTENSION` (0x03) chunk is consumed as a whole: the dispatcher walks the plugin chunks
  it contains, calls each registered reader with `(length, object, offset, size)`, skips whatever
  the reader left unread, and finally skips any bytes left in the extension.
- A reader must consume exactly its declared `length` and must not expect a trailing dword; a
  zero-length plugin chunk is legal and advances the stream by exactly the 12-byte chunk header
  (type, length, library id). Reading past the length is reported as
  `Rw: plugin 0x... read past its chunk`; reading too little is realigned by the dispatcher. Both
  rules exist so that a length-0 chunk or an extra dword cannot desynchronise the stream.
- Extension chunks are optional (`ReadPlugins` peeks at the header): a missing one is legal, but a
  present one must be consumed here. `RpLightStreamRead` finds and consumes its own `rwID_LIGHT`
  chunk, so a caller must not find it first - doing that made every light-bearing model fail to
  load (`RwDff.cpp:544`).

`BreakablePlugin` (`source/game_sa/Plugins/BreakablePlugin/BreakablePlugin.cpp`) keeps two
layouts: `BreakInfoDisk` is exactly `0x34` bytes with 32-bit pointer fields (retail
`BreakableStreamGetSize`, `0x59D0F0`), while the runtime `BreakInfo_t` widens those to native
pointers. Read and write through `BreakInfoDisk` and rebuild the pointers; never `RwStreamRead` or
`RwStreamWrite` the runtime struct.

### Plugin slots are pointer-sized

The game registers one-pointer plugin areas with the retail literal `4`
(`CCustomCarEnvMapPipeline::RegisterPlugin`, `0x5DA450`). A 4-byte slot makes the constructor
write the upper half of a 64-bit pointer past the object. Both sides carry the fix:

- `source/game_sa/Pipelines/CustomCarEnvMap/CustomCarEnvMapPipeline.cpp:62-89`:
  `kEnvMapMatPluginSize`, `kEnvMapAtmPluginSize` and `kSpecMapMatPluginSize` are `sizeof(...*)`.
- `PluginRegistry::Register` (`source/game_sa/RenderWare/librw/RwFakerw.cpp:154`) aligns every
  slot and stores at least `sizeof(void*)` bytes, while reporting the *registered* size to the
  callbacks and stream readers. A game-side `4` can therefore no longer clobber the next slot,
  but it still yields a wrong logical size, so fix the registration, not just the padding.

### Executable globals

Fixed addresses are accessed through `StaticRef<T>(0xADDR)`
(`source/app/platform/native/ExeGlobals.h`), backed by zeroed per-address storage; initial values
come from `ApplyExeData()` (see *Exe static initializers*). A raw retail address in ported code
has no mapping natively: it either faults or, worse, reads zeros silently.

- Treat every absolute operand in a retail function as a candidate global and declare it as a
  `StaticRef` in its owning header. Pointer-valued globals cannot be replayed from initializer
  bytes and need the typed `...X` container (see *File-backed `.data` with no initializer entry*).
- Not every fixed address is a standalone global. `CStreaming::IsModelLoaded`
  (`source/game_sa/Streaming.h:423`) is `ms_aInfoForModel[model].m_LoadState`, and
  `CStreamingInfo::ms_pArrayBase` (`0x9654B4`, `StreamingInfo.h:50`) is an internal pointer into
  that same array, assigned in `CStreaming::Init2` (`Streaming.cpp:2802`). Giving them independent
  storage yields two disjoint copies of the streaming table; the array is authoritative.

### Freeing game memory

Blocks from `CMemoryMgr::Malloc` (and from the scratch pad) are accounted by the native memory
manager, so they must be released with `CMemoryMgr::Free` (`source/game_sa/MemoryMgr.cpp:422`),
which drops the block's `g_blocks` entry and returns scratch-pad memory to the pad.
`delete`/`delete[]` bypasses that accounting.

- `CCollisionData` single-allocation buffers are placement-new over `CMemoryMgr::Malloc`
  (`CColModel::AllocateData(int32)`, `source/game_sa/Collision/ColModel.cpp:127`) and are freed
  with `CMemoryMgr::Free` in `MakeMultipleAlloc` (`:69`). `RemoveCollisionVolumes` keeps the same
  split on `m_bIsSingleColDataAlloc`; only the multi-allocation case uses `delete`.
- Rasters go through `RwRasterDestroy` (`source/game_sa/RenderWare/librw/RwCamera.cpp:354`), which
  destroys the librw mirror and then frees the game object through `notsa::fakerw::FreeObject`
  (plugin-aware).

### Interiors

`Interior_c` (`source/game_sa/Interior/Interior_c.h`) is one of the largest classes in the port:

- `PlaceFurniture` returns `CEntity*` (`Interior_c.h:122`, body `Interior_c.cpp:193`);
  `Office_PlaceEdgeFillers` returns `int32` (`Interior_c.h:86`, body `Interior_c.cpp:338`). The
  `Furnish*`/`Shop_*` helpers consume those results, so their types are part of the interface.
- The class models the interior graph as `m_numGotoPoints` + `m_gotoPoints[16]` (`0x410`),
  `m_doorPathNodes[4]` (`0x510`) and `m_interiorInfos[16]` (`0x590`). The `VALIDATE_OFFSET` /
  `VALIDATE_SIZE` calls after the class document the retail 32-bit layout; they are compiled out
  under `NOTSA_NATIVE` (`source/Base.h:29-35`), and the class itself widens (it derives from
  `ListItem_c`), so access fields through their names and clamp counts (`m_numGotoPoints`,
  `m_interiorInfosCount`) to the array sizes when reading world data. `AddGotoPt`
  (`Interior_c.cpp:1304`) already rejects `>= 16`.

### FX particle layout

`FxEmitterPrt_c` (`source/game_sa/Fx/FxEmitterPrt.h`) is `0x3C` bytes in the retail layout, with
`m_MultColor` at `+0x2C`, `m_RotZ` at `+0x36` and `m_bLocalToSystem` at `+0x37` (constructor
`0x4A2580`, checked by the `VALIDATE_SIZE`/`VALIDATE_OFFSET` calls there); `Particle_c` is `0x2C`
with `m_System` at `+0x28`. Those assertions are Windows-only and both classes widen natively
(`ListItem_c` holds two pointers, `Particle_c::m_System` is another), so address particle fields
by name, never by the retail offset. Allocation goes through `FxEmitterPrt_c::operator new[]`
(`FxEmitterPrt.cpp`), which passes `sizeof(FxEmitterPrt_c)` to `g_fxMan.GetMemPool()`.

## Exe static initializers

The executable's `_cinit` (`0x823B76`) walks the C++ initializer table `__xi_a..__xi_z`
(`0x8A4000..0x8A5A10`, 1667 non-null entries) and calls every entry before `WinMain`. For the
`.data` tail those entries are the only definition of the initial values, so the native build has
to replay them.

`tools/native/gen_exedata.py` does that mechanically. It reads the table from `$GTASA_EXE` (or
`--exe`), decodes every initializer body as x86-32 (`capstone`; `llvm-objdump` when capstone is
missing - both produce the same table), and emits

- `source/app/platform/native/ExeData.generated.{h,cpp}` - one `EXEDATA_ROW(kind, addr, aux, size,
  count, value)` per effect, in table order;
- `source/app/platform/native/ExeData.manual.{h,cpp}` - typed construction of the objects whose
  constructor is already reversed (hand written, see below).

`notsa::native::ApplyExeData()` (called from `main` right after `ExeGlobals::ReportOverlaps()`)
replays the table through `ExeGlobals`; the startup log line is
`ExeData: applied 1513 rows (18200 bytes), 5 manual, 54 skipped, 96 constructors not applied, 3 unhandled`.

### File-backed `.data` with no initializer entry

Only the initializer table is replayed, so a table the executable carries as raw `.data` bytes and
never initializes is all-zero in the native build. When such a table holds pointers (and its 32-bit
layout is therefore unusable anyway), the C++ `...X` replacement declared next to the `StaticRef`
has to be used directly instead of the executable address:

```cpp
#ifdef NOTSA_NATIVE
    static inline auto& ms_aAnimAssocDefinitions = ms_aAnimAssocDefinitionsX;
#else
    static inline auto& ms_aAnimAssocDefinitions = StaticRef<AnimAssocDefinition[...]>(0x8AA5A8);
#endif
```

Size the replacement for the native element layout and for what the game appends at runtime
(`CAnimManager::ms_aAnimAssocDefinitionsX` needs `ANIM_TOTAL_GROUPS` entries, not
`NUM_ANIM_ASSOC_GROUPS`: `ANIMGRP.DAT` adds `ANIM_GROUP_MAN`..`ANIM_GROUP_SKATE`). Do not keep the
executable address with a widened array - the native stride would run into the next globals
(`CutsceneMgr::ms_useCutsceneShadows` at `0x8AC158` and `AEAmbienceTrackManager` at `0x8AC15C`
for the animation definitions).

### How an entry is decoded

Each body runs in a small concrete interpreter (registers incl. 8/16-bit halves, stack, flags,
`rep stosd`, and the x87 subset the table uses). It starts from the executable's own memory (file
bytes, zero `.bss`) and runs the entries in table order, so an operand that an earlier initializer
wrote is read back with its runtime value. x87 arithmetic is exact (`fractions.Fraction`) and the
result is rounded once to `float32`, which is what `fstp dword` does. Anything that cannot be
modelled (an indirect call, a value from an unknown register, a non-exact `_ftol`, `fcos` of a
non-zero angle) makes the *whole* entry `Unhandled` and its writes are rolled back, so a partly
understood body never emits guessed values.

A write whose bytes already equal the native starting state (all-zero object) is dropped, which is
why the executable's `memset`-style zero initialization collapses to nothing; `rep stosd` of zeros
is kept as `ZeroFill` because it clears a region explicitly.

### Counts (compact exe, sha256 `8ebc7ff3...`)

| rows | kind | what it is |
| --- | --- | --- |
| 1489 | `StoreFloat` | x87 constant chains (`fld`/`fmul`/`fdiv`/`fdivr`/`fsub`/`fild` -> `fstp dword`) |
| 10 | `StoreBytes` | integer constants and copies of another global |
| 9 | `FillPattern` | `mov word [reg], 0xffff` loops, `rep stosd` of a non-zero value, constant array-element constructors |
| 5 | `ZeroFill` | `rep stosd` of zero |
| 81 | `ObjectCtor` | `mov ecx,<global>` + `call`/`jmp <ctor>`; not applied automatically |
| 19 | `VectorCtor` | `_eh_vector_constructor_iterator` with a non-trivial element constructor |
| 54 | `Skip` | 20 `_atexit`-only entries, 20 array constructors whose element constructor only clears, 14 entries whose writes are already zero |
| 4 | `Unhandled` | see below |

1671 rows for 1667 entries (an entry can contribute several writes).

### Hand-written initializers (`ExeData.manual.cpp`)

| entry | global | what runs |
| --- | --- | --- |
| `0x84D920` | `CControllerConfigManager` (`0xB70198`) | our reversed constructor (`ControllerConfigManager.cpp:80`) |
| `0x84D5D0` | `CAudioEngine` (`0xB6BC90`) | compiler-generated constructor (`AudioEngine.h:42`); only its audio entity members' vtables need it |
| `0x84E1F0` | `CPad::Pads` (`0xB73458`, 2 x 0x134) | our reversed `CPad::CPad` (`Pad.cpp:125`) |
| `0x84E9B0` | `CWorld::ms_aSectors` (`0xB7D0B8`) | clear; element constructor `0x564040` only zeroes |
| `0x84E9E0` | `CWorld::ms_aRepeatSectors` (`0xB992B8`) | clear; `0x563190` only clears three inline list heads |
| `0x84EA10` | `CWorld::ms_aLodPtrLists` (`0xB99EB8`) | clear; `0x43E3F0` only clears |
| `0x8543D0` | `gHandlingDataMgr` (`0xC2B9C8`) | `memset` of the 0xC624 bytes the executable clears |

`CWorld::Players` (`0xB7CD98`) is deliberately **not** applied: its element constructor `0x571920`
builds `m_PlayerData` through `0x56F210`, and our `CPlayerInfo::CPlayerInfo` (`PlayerInfo.cpp:43`)
still forwards to `0x571920` (which aborts under `NOTSA_NATIVE`). Reversing that constructor
unblocks it.

### Still unhandled

| entry | reason |
| --- | --- |
| `0x8339CA` | CRT `__security_cookie`; calls through the IAT, no game state |
| `0x84D750` | stores the address `0x8CC958` (computed from a byte) at `0xB6FE88`; a raw executable address has no meaning natively |
| `0x852840` | `fcos` of pi/8; the x87 cosine approximation is not the correctly rounded value, so the result is not guessed |

Known limitation: `ExeGlobals` reserves storage per address, so an initializer write that lands
*inside* another `StaticRef`'s object gets its own buffer until the two references are unified
(`ReportOverlaps` lists every such pair). A single arena for the whole `.data` section was tried:
it reproduces the retail layout exactly, but it makes `gStreamFileHandles` (32 eight-byte
`HANDLE`s at `0x8E4010`) collide with `gStreamCount`/`gOpenStreamCount`
(`0x8E4090`/`0x8E4094`) and `gCdImageNames` (`0x8E4098`), which are four bytes apart in the
32-bit layout. That is a real 64-bit layout bug (`CdStreamInfo.cpp:7-9`) and belongs to B5.

`python3 tools/native/gen_exedata.py --check` regenerates in memory and fails when the checked-in
files are out of date. `tools/native/staticref_report.py --check` also reads the rows (and still
reports the 53 file-backed `.data` addresses, which need a copy of their initial value rather than
an initializer).

Differences from `.local/re/initterm.md`, which evaluated float operands from the file instead of
the running table state: 233 float rows differ, e.g. `0x848F90` multiplies `0x8E3EEC`, which entry
`0x848F70` already set to 0.0004 (result 0.004, not 0); one row inverts `fdivr` (`0x84CFC0` reports
`inf` where the executable computes 2.0/150 = 0.0133); `0x84C7A0` is listed although both operands
are 0.56 (the result is zero).

## Verifying a native change

Syntax check first; it needs `build/Release/compile_commands.json` but does not write into the
build tree:

```sh
python3 tools/native/syntax_check.py source/game_sa/<file>.cpp
```

It compiles the file with its recorded flags (minus the PCH), so it catches type and header
errors, not link errors.

Then a scripted new game for 900 frames, with a screenshot:

```sh
cd ~/gtasa/gta-reversed
: > ~/Games/GTASA/logs/log.log
GTASA_NEW_GAME=1 GTASA_SKIP_CUTSCENES=1 GTASA_CAPTURE=/tmp/ng.png GTASA_CAPTURE_FRAME=900 \
  timeout -s KILL 90 bin/Release/gta_reversed.app/Contents/MacOS/gta_reversed \
  -ApplePersistenceIgnoreState YES --game-dir ~/Games/GTASA >/tmp/run.txt 2>&1; echo EXIT=$?
```

- `-ApplePersistenceIgnoreState YES` is not optional: without it macOS shows the window
  restoration dialog and the game never advances.
- `timeout` is GNU coreutils (`brew install coreutils`; use `gtimeout` when the `gnubin`
  symlinks are not on `PATH`).
- Pass: `EXIT=0` with a 3D scene in `/tmp/ng.png`, or `EXIT=124`/`137` (killed by `timeout`) with
  no `Native: fatal signal` in `/tmp/run.txt` and no `Unreversed` line in the log. The log tail
  says where the run stopped.

`NativeCrash.cpp` prints `Native: fatal signal <n> at <address> (pc <pc>)` plus
`backtrace_symbols_fd`; only the signals it installs are reported (currently `SIGSEGV`, `SIGBUS`,
`SIGILL`, `SIGFPE`, `SIGABRT`, `SIGTRAP`). The binary is built with `-g0`
(`source/CMakeLists.txt:138`), so there is no DWARF. The printed PC is a runtime address and the
app is PIE, so either disable ASLR under lldb or subtract the image base (the default `__TEXT`
vmaddr is `0x100000000`), then resolve it with `atos -o <binary> -l <base> <pc>` or by finding
the nearest symbol in `nm -nm <binary>` and disassembling it with
`objdump -d --start-address=0x<addr> --stop-address=0x<end> <binary>`.

## Packaging

`python3 setup.py --build -p macos-arm64.txt` produces `bin/<config>/gta_reversed.app`.

- `source/app/platform/native/Info.plist.in` is configured into the build tree
  (`configure_file`, `@ONLY`) and installed as the bundle's `Info.plist` through
  `MACOSX_BUNDLE_INFO_PLIST`: `CFBundleIdentifier=io.github.nic0der-im.gtareversed`,
  `CFBundleName=GTA San Andreas (reversed)`, `NSHighResolutionCapable=true` and
  `LSMinimumSystemVersion=12.0`.
- A `POST_BUILD` step runs `codesign --force --deep -s - gta_reversed.app`, so the bundle launches
  without a developer certificate. A warning is emitted instead when `codesign` isn't installed.

The game directory is resolved by `notsa::native::NativeGameDir::Resolve`
(`app/platform/native/NativeGameDir.{h,cpp}`), in this order:

1. `--game-dir <path>`
2. the `GTASA_DIR` environment variable
3. the path saved by an earlier run in `gamedir.txt` inside
   `SDL_GetPrefPath("gta-reversed", "gtasa")` (`~/Library/Application Support/gta-reversed/gtasa/`)
4. a native folder picker (`SDL_ShowOpenFolderDialog`, video subsystem initialized on demand, five
   minute cap); a successful choice is written back to `gamedir.txt`

Every candidate must contain `models/gta3.img`, matched case-insensitively through `NormalizePath`.
A bad explicit `--game-dir`/`GTASA_DIR` is reported instead of falling through to the picker; when
no source yields a directory the game shows `Pass --game-dir or set GTASA_DIR` and exits with 1.
