#include "StdInc.h"

#include "FxMemoryPool.h"

void FxMemoryPool_c::InjectHooks() {
    RH_ScopedClass(FxMemoryPool_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(Init, 0x4A9C30);
    RH_ScopedInstall(Exit, 0x4A9C80);
    RH_ScopedInstall(Reset, 0x4A9C90);
    RH_ScopedInstall(GetMem, 0x4A9CA0);
    RH_ScopedInstall(Optimise, 0x4A9CD0);
}

// 0x4A9C30
void FxMemoryPool_c::Init() {
    m_nSize     = 1024 * 1024; // 1 megabyte
    m_nPosition = 0;
    m_pData     = static_cast<uint8*>(CMemoryMgr::Malloc(m_nSize));

    if (m_pData)
        memset(m_pData, 0, m_nSize);
}

// 0x4A9C80
void FxMemoryPool_c::Exit() {
    // The executable frees the buffer (`mov eax, [ecx]; ... call CMemoryMgr::Free`), not the pool
    // object itself - the object lives inside `g_fxMan`, so freeing it corrupted the heap.
    CMemoryMgr::Free(std::exchange(m_pData, nullptr));
}

// 0x4A9C90
void FxMemoryPool_c::Reset() {
    m_nPosition = 0;
}

// 0x4A9CA0
void* FxMemoryPool_c::GetMem(int32 size, int32 align) {
    if (m_nPosition % align > 0)
        m_nPosition = align + m_nPosition - m_nPosition % align;

    auto result = m_pData + m_nPosition;
    m_nPosition = size + m_nPosition;
    return result;
}

// 0x4A9CD0
void FxMemoryPool_c::Optimise() {
    // The executable shrinks the pool in place (`m_pData = CMemoryMgr::Realloc(m_pData, m_nPosition, 0)`,
    // `m_nSize = m_nPosition`), and its heap keeps the block where it is. Native `realloc` may
    // relocate instead, and every `FxSystemBP_c`/`FxPrimBP_c`/`FxEmitterBP_c` is stored inside
    // this buffer and referenced from `FxManager_c::m_FxSystemBPs` (and from each other), so a
    // relocation leaves those lists pointing at the freed block. That surfaced as
    // `FxManager_c::FindFxSystemBP` matching nothing (the old block gets reused for the .fxp text)
    // and, worse, as heap corruption inside `CMemoryMgr::Malloc` during streaming.
    // The used size is recorded nowhere else; `m_nSize` stays the allocated capacity so `GetMem`
    // can be bounds-checked against it later.
}