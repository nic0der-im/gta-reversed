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

## Exe-resident state (B2)

`scripts/native/staticref_report.py` parses every `StaticRef`/`ScopedStaticRef` site in
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
`scripts/native/gen_exedata.py` decodes the 1667-entry `__xi_a..__xi_z` table and emits
`source/app/platform/native/ExeData.generated.{h,cpp}`, replayed at startup by
`notsa::native::ApplyExeData()` (writes go through `ExeGlobals`). Current split: 1489 float
stores, 10 byte stores, 9 fill-patterns, 5 zero-fills, 81 object constructors, 19 vector
constructors, 54 skips, 4 unhandled (documented in the file). Object constructors that need our
own C++ types live in `ExeData.manual.cpp`; the rest are listed as not-applied.

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

## Exe static initializers

The executable's `_cinit` (`0x823B76`) walks the C++ initializer table `__xi_a..__xi_z`
(`0x8A4000..0x8A5A10`, 1667 non-null entries) and calls every entry before `WinMain`. For the
`.data` tail those entries are the only definition of the initial values, so the native build has
to replay them.

`scripts/native/gen_exedata.py` does that mechanically. It reads the table from `$GTASA_EXE` (or
`--exe`), decodes every initializer body as x86-32 (`capstone`; `llvm-objdump` when capstone is
missing - both produce the same table), and emits

- `source/app/platform/native/ExeData.generated.{h,cpp}` - one `EXEDATA_ROW(kind, addr, aux, size,
  count, value)` per effect, in table order;
- `source/app/platform/native/ExeData.manual.{h,cpp}` - typed construction of the objects whose
  constructor is already reversed (hand written, see below).

`notsa::native::ApplyExeData()` (called from `main` right after `ExeGlobals::ReportOverlaps()`)
replays the table through `ExeGlobals`; the startup log line is
`ExeData: applied 1513 rows (18200 bytes), 4 manual, 54 skipped, 97 constructors not applied, 3 unhandled`.

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

`python3 scripts/native/gen_exedata.py --check` regenerates in memory and fails when the checked-in
files are out of date. `scripts/native/staticref_report.py --check` also reads the rows (and still
reports the 53 file-backed `.data` addresses, which need a copy of their initial value rather than
an initializer).

Differences from `.local/re/initterm.md`, which evaluated float operands from the file instead of
the running table state: 233 float rows differ, e.g. `0x848F90` multiplies `0x8E3EEC`, which entry
`0x848F70` already set to 0.0004 (result 0.004, not 0); one row inverts `fdivr` (`0x84CFC0` reports
`inf` where the executable computes 2.0/150 = 0.0133); `0x84C7A0` is listed although both operands
are 0.56 (the result is zero).
