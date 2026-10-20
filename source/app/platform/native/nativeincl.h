#pragma once

/*
 * Native (macOS/arm64) counterpart of `app/platform/win/winincl.h`, included from
 * `StdInc.h` when the build has `NOTSA_NATIVE` defined.
 *
 * Only *types* and calling convention shims live here - Win32 *functions* are never
 * emulated. Any file that calls the Win32 API must have its own `#ifdef NOTSA_NATIVE`
 * branch (see `CFileMgr::OpenFile`, `NativeMain.cpp`, ...).
 *
 * This header is the first platform header included by every translation unit, so it
 * must stay self contained and cheap.
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <unistd.h>

#include "crt_compat.h"

//! x86 calling conventions - They have no meaning on arm64, so they're defined away.
#define __cdecl
#define __stdcall
#define __thiscall
#define __fastcall
#define WINAPI
#define WINAPIV
#define CALLBACK

//! Opaque Direct3D9 (COM) interfaces - The code base only handles them by pointer.
//! See plan B3, where `librw` replaces the D3D9 renderer entirely.
struct IDirect3DDevice9;
struct IDirect3DIndexBuffer9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DSurface9;
struct IDirect3DTexture9;

//! D3D9 primitive types - `RxD3D9ResEntryHeader` stores one by value, so the enum
//! (and not just a forward declaration) is needed. Values match d3d9.h.
enum D3DPRIMITIVETYPE : std::int32_t {
    D3DPT_POINTLIST     = 1,
    D3DPT_LINELIST      = 2,
    D3DPT_LINESTRIP     = 3,
    D3DPT_TRIANGLELIST  = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN   = 6,
};

//! Opaque DirectInput interfaces (Referenced by `psGlobalType`, never used natively)
struct IDirectInput8;
struct IDirectInputDevice8;

using LPDIRECTINPUT8       = IDirectInput8*;
using LPDIRECTINPUTDEVICE8 = IDirectInputDevice8*;

//! Win32 type aliases.
//! Only the ones demanded by headers shared with the Windows build are provided.
using HANDLE     = void*;
using HWND       = void*;
using HINSTANCE  = void*;
using LPVOID     = void*;
using CHAR       = char;
using BYTE       = std::uint8_t;
using WORD       = std::uint16_t;
using DWORD      = std::uint32_t;
using BOOL       = std::int32_t;
using HRESULT    = std::int32_t;

//! Windows' `MAX_PATH` - Used as the size of several game path buffers
constexpr std::int32_t MAX_PATH = 260;

//! Virtual key codes (Values taken from the Windows SDK). Only constants are provided -
//! There's no `GetAsyncKeyState`; input comes from SDL natively.
enum : std::int32_t {
    VK_BACK     = 0x08,
    VK_TAB      = 0x09,
    VK_RETURN   = 0x0D,
    VK_SHIFT    = 0x10,
    VK_CONTROL  = 0x11,
    VK_MENU     = 0x12,
    VK_PAUSE    = 0x13,
    VK_CAPITAL  = 0x14,
    VK_ESCAPE   = 0x1B,
    VK_SPACE    = 0x20,
    VK_PRIOR    = 0x21,
    VK_NEXT     = 0x22,
    VK_END      = 0x23,
    VK_HOME     = 0x24,
    VK_LEFT     = 0x25,
    VK_UP       = 0x26,
    VK_RIGHT    = 0x27,
    VK_DOWN     = 0x28,
    VK_INSERT   = 0x2D,
    VK_DELETE   = 0x2E,
    VK_LWIN     = 0x5B,
    VK_RWIN     = 0x5C,
    VK_APPS     = 0x5D,
    VK_NUMPAD0  = 0x60,
    VK_NUMPAD1  = 0x61,
    VK_NUMPAD2  = 0x62,
    VK_NUMPAD3  = 0x63,
    VK_NUMPAD4  = 0x64,
    VK_NUMPAD5  = 0x65,
    VK_NUMPAD6  = 0x66,
    VK_NUMPAD7  = 0x67,
    VK_NUMPAD8  = 0x68,
    VK_NUMPAD9  = 0x69,
    VK_MULTIPLY = 0x6A,
    VK_ADD      = 0x6B,
    VK_SUBTRACT = 0x6D,
    VK_DECIMAL  = 0x6E,
    VK_DIVIDE   = 0x6F,
    VK_F1       = 0x70,
    VK_F2       = 0x71,
    VK_F3       = 0x72,
    VK_F4       = 0x73,
    VK_F5       = 0x74,
    VK_F6       = 0x75,
    VK_F7       = 0x76,
    VK_F8       = 0x77,
    VK_F9       = 0x78,
    VK_F10      = 0x79,
    VK_F11      = 0x7A,
    VK_F12      = 0x7B,
    VK_NUMLOCK  = 0x90,
    VK_SCROLL   = 0x91,
    VK_LSHIFT   = 0xA0,
    VK_RSHIFT   = 0xA1,
};

//! Used by the save game structures (`SimpleVariablesSaveStructure`).
struct SYSTEMTIME {
    std::uint16_t wYear;
    std::uint16_t wMonth;
    std::uint16_t wDayOfWeek;
    std::uint16_t wDay;
    std::uint16_t wHour;
    std::uint16_t wMinute;
    std::uint16_t wSecond;
    std::uint16_t wMilliseconds;
};
static_assert(sizeof(SYSTEMTIME) == 16, "SYSTEMTIME must stay 16 bytes for PC compatible save games");
