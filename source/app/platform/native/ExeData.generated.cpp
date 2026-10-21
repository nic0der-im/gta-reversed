//! @file ExeData.generated.cpp
//! The executable's CRT static-initializer effects, ready to replay into `ExeGlobals`.
//!
//! GENERATED FILE - DO NOT EDIT. Regenerate with `python3 scripts/native/gen_exedata.py`.
//! Source: the `__xi_a..__xi_z` table (`0x8A4000..0x8A5A10`, 1667 entries) of `gta_sa.exe`.
//! Row kinds and their fields are documented in `ExeData.generated.h`.

#ifdef NOTSA_NATIVE

#include "app/platform/native/ExeData.generated.h"

#include <array>
#include <cstddef>

#include <spdlog/spdlog.h>

#include "app/platform/native/ExeData.manual.h"
#include "app/platform/native/ExeGlobals.h"

namespace notsa::native {
namespace {

//! One table row: `kind, addr, aux, size, count, value`.
#define EXEDATA_ROW(kind, addr, aux, size, count, value) {ExeDataKind::kind, addr, aux, size, count, value}

constexpr ExeDataEntry kEntries[] = {
    /* 0x8339CA indirect call */
    EXEDATA_ROW(Unhandled, 0x8339CA, 0x0, 0, 0, 0x0), // indirect call
    /* 0x848F10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EE0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x848F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EE4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x848F50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EE8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x848F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EEC, 0x0, 4, 1, 0x39D1B717),
    /* 0x848F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EF0, 0x0, 4, 1, 0x3B83126E),
    /* 0x848FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3EF4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x848FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FB8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x848FF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FBC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FC0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FC4, 0x0, 4, 1, 0x39D1B717),
    /* 0x849050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FC8, 0x0, 4, 1, 0x3B83126E),
    /* 0x849070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x8E3FCC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654D8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8490B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654DC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8490D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654E0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8490F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654E4, 0x0, 4, 1, 0x39D1B717),
    /* 0x849110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654E8, 0x0, 4, 1, 0x3B83126E),
    /* 0x849130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9654EC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96553C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965540, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849190 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965544, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8491B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965548, 0x0, 4, 1, 0x39D1B717),
    /* 0x8491D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96554C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8491F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965550, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965568, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96556C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965570, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849270 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965574, 0x0, 4, 1, 0x39D1B717),
    /* 0x849290 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x965578, 0x0, 4, 1, 0x3B83126E),
    /* 0x8492B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96557C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8492D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655E8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8492F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655EC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849310 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655F0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849330 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655F4, 0x0, 4, 1, 0x39D1B717),
    /* 0x849350 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655F8, 0x0, 4, 1, 0x3B83126E),
    /* 0x849370 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9655FC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849390 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689E4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8493B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689E8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8493D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689EC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8493F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689F0, 0x0, 4, 1, 0x39D1B717),
    /* 0x849410 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689F4, 0x0, 4, 1, 0x3B83126E),
    /* 0x849430 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689F8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9689FC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849470 mov ecx, 0x968a00 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968A00, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968a00 ; call 0x40fb60
    /* 0x849490 _eh_vector_constructor_iterator(ctor 0x40FB60 dtor 0x40F700 20 x 0x30) */
    EXEDATA_ROW(VectorCtor, 0x968A30, 0x40FB60, 48, 20, 0x0), // _eh_vector_constructor_iterator(ctor 0x40FB60 dtor 0x40F700 20 x 0x30)
    /* 0x8494C0 mov ecx, 0x968df0 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968DF0, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968df0 ; call 0x40fb60
    /* 0x8494E0 mov ecx, 0x968e20 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968E20, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968e20 ; call 0x40fb60
    /* 0x849500 mov ecx, 0x968e50 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968E50, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968e50 ; call 0x40fb60
    /* 0x849520 mov ecx, 0x968e80 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968E80, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968e80 ; call 0x40fb60
    /* 0x849540 mov ecx, 0x968eb0 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968EB0, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968eb0 ; call 0x40fb60
    /* 0x849560 mov ecx, 0x968ee0 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968EE0, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968ee0 ; call 0x40fb60
    /* 0x849580 mov ecx, 0x968f10 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968F10, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968f10 ; call 0x40fb60
    /* 0x8495A0 mov ecx, 0x968f40 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968F40, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968f40 ; call 0x40fb60
    /* 0x8495C0 mov ecx, 0x968f70 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968F70, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968f70 ; call 0x40fb60
    /* 0x8495E0 mov ecx, 0x968fa0 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968FA0, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968fa0 ; call 0x40fb60
    /* 0x849600 mov ecx, 0x968fd0 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0x968FD0, 0x40FB60, 0, 0, 0x0), // mov ecx, 0x968fd0 ; call 0x40fb60
    /* 0x849620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969000, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969004, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969008, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96900C, 0x0, 4, 1, 0x39D1B717),
    /* 0x8496A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969010, 0x0, 4, 1, 0x3B83126E),
    /* 0x8496C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969014, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8496E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969040, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969044, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969048, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96904C, 0x0, 4, 1, 0x39D1B717),
    /* 0x849760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969050, 0x0, 4, 1, 0x3B83126E),
    /* 0x849780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969054, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8497A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969058, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8497C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96905C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8497E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969060, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969064, 0x0, 4, 1, 0x39D1B717),
    /* 0x849820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969068, 0x0, 4, 1, 0x3B83126E),
    /* 0x849840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96906C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690D0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690D4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8498A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690D8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8498C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690DC, 0x0, 4, 1, 0x39D1B717),
    /* 0x8498E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690E0, 0x0, 4, 1, 0x3B83126E),
    /* 0x849900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9690E4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969194, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969198, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96919C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9691A0, 0x0, 4, 1, 0x39D1B717),
    /* 0x8499A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9691A4, 0x0, 4, 1, 0x3B83126E),
    /* 0x8499C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x9691A8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8499E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A24, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849A00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A28, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849A20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A2C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849A40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A30, 0x0, 4, 1, 0x39D1B717),
    /* 0x849A60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A34, 0x0, 4, 1, 0x3B83126E),
    /* 0x849A80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x969A38, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849AA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A708, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849AC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A70C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849AE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A710, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849B00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A714, 0x0, 4, 1, 0x39D1B717),
    /* 0x849B20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A718, 0x0, 4, 1, 0x3B83126E),
    /* 0x849B40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A71C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849B60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7E4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849B80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7E8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849BA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7EC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849BC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7F0, 0x0, 4, 1, 0x39D1B717),
    /* 0x849BE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7F4, 0x0, 4, 1, 0x3B83126E),
    /* 0x849C00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A7F8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849C20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849C40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8C0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849C60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8C4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849C80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8C8, 0x0, 4, 1, 0x39D1B717),
    /* 0x849CA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8CC, 0x0, 4, 1, 0x3B83126E),
    /* 0x849CC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96A8D0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849CE0 _eh_vector_constructor_iterator(ctor 0x441E00 dtor 0x4411A0 13 x 0x1C) */
    EXEDATA_ROW(VectorCtor, 0x96A9B8, 0x441E00, 28, 13, 0x0), // _eh_vector_constructor_iterator(ctor 0x441E00 dtor 0x4411A0 13 x 0x1C)
    /* 0x849D10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABA4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849D30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABA8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849D50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABAC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849D70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABB0, 0x0, 4, 1, 0x39D1B717),
    /* 0x849D90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABB4, 0x0, 4, 1, 0x3B83126E),
    /* 0x849DB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96ABB8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849DD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C02C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849DF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C030, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849E10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C034, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849E30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C038, 0x0, 4, 1, 0x39D1B717),
    /* 0x849E50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C03C, 0x0, 4, 1, 0x3B83126E),
    /* 0x849E70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96C040, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849E90 _eh_vector_constructor_iterator(ctor 0x4470E0 dtor 0x447110 50 x 0xD8) */
    EXEDATA_ROW(VectorCtor, 0x96C048, 0x4470E0, 216, 50, 0x0), // _eh_vector_constructor_iterator(ctor 0x4470E0 dtor 0x447110 50 x 0xD8)
    /* 0x849EC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA7C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849EE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA80, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849F00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA84, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849F20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA88, 0x0, 4, 1, 0x39D1B717),
    /* 0x849F40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA8C, 0x0, 4, 1, 0x3B83126E),
    /* 0x849F60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96EA90, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x849F80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F034, 0x0, 4, 1, 0x39A3D70A),
    /* 0x849FA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F038, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x849FC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F03C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x849FE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F040, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A000 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F044, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x96F048, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A040 1 row(s) */
    EXEDATA_ROW(FillPattern, 0x972CD0, 0x4, 2, 5000, 0xFFFF), // pattern x5000 stride 0x4
    /* 0x84A060 1 row(s) */
    EXEDATA_ROW(FillPattern, 0x977AF0, 0x4, 2, 32, 0xFFFF), // pattern x32 stride 0x4
    /* 0x84A080 1 row(s) */
    EXEDATA_ROW(FillPattern, 0x977B7C, 0x4, 2, 8, 0xFFFF), // pattern x8 stride 0x4
    /* 0x84A0A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BA0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A0C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BA4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A0E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BA8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A100 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BAC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A120 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BB0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BB4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BCC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BD0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A1A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BD4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A1C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BD8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A1E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BDC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A200 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x977BE0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A220 mov ecx, 0x977be8 ; call 0x4541e0 */
    EXEDATA_ROW(ObjectCtor, 0x977BE8, 0x4541E0, 0, 0, 0x0), // mov ecx, 0x977be8 ; call 0x4541e0
    /* 0x84A240 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D648, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A260 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D64C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A280 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D650, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A2A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D654, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A2C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D658, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A2E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97D65C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A300 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F634, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A320 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F638, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F63C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F640, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F644, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A3A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F648, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A3C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F64C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A3E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F650, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F654, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F658, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F65C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0x97F660, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43098, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A4A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA4309C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A4C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA430A0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A4E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA430A4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA430A8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA430AC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A540 mov ecx, 0xa430b0 ; call 0x571920 */
    EXEDATA_ROW(ObjectCtor, 0xA430B0, 0x571920, 0, 0, 0x0), // mov ecx, 0xa430b0 ; call 0x571920
    /* 0x84A560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43310, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43588, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A5A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA4358C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A5C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43590, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A5E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43594, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA43598, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA4359C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A640 1 row(s) */
    EXEDATA_ROW(FillPattern, 0xA435A0, 0x4, 2, 325, 0xFFFF), // pattern x325 stride 0x4
    /* 0x84A660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA90834, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA90838, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A6A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9083C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A6C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA90840, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A6E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA90844, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA90848, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A720 150 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84A720, 0x0, 0, 0, 0x0), // 150 write(s) already zero in the native build
    /* 0x84A740 mov ecx, 0xa90cf0 ; jmp 0x46b260 */
    EXEDATA_ROW(ObjectCtor, 0xA90CF0, 0x46B260, 0, 0, 0x0), // mov ecx, 0xa90cf0 ; jmp 0x46b260
    /* 0x84A750 _eh_vector_constructor_iterator(ctor 0x463750 dtor 0x463760 96 x 0x44) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84A750, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x463750 dtor 0x463760 96 x 0x44) (element ctor only clears)
    /* 0x84A780 _eh_vector_constructor_iterator(ctor 0x463770 dtor 0x463780 128 x 0x3C) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84A780, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x463770 dtor 0x463780 128 x 0x3C) (element ctor only clears)
    /* 0x84A7B0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 128 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84A7B0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 128 x 0x4) (element ctor only clears)
    /* 0x84A7E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95148, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9514C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95150, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95154, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95158, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9515C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A8A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95160, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A8C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95164, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A8E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95168, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9516C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95170, 0x0, 4, 1, 0x3B83126E),
    /* 0x84A940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95174, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84A960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95178, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84A980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9517C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84A9A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95180, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84A9C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95184, 0x0, 4, 1, 0x39D1B717),
    /* 0x84A9E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95188, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AA00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9518C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AA20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95194, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AA40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95198, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AA60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9519C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AA80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951A0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AAA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951A4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AAC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951A8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AAE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951AC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AB00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951B0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AB20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951B4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AB40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951B8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AB60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951BC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AB80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951C0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84ABA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951C4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84ABC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951C8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84ABE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951CC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AC00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951D0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AC20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951D4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AC40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951D8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AC60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951DC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AC80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951E0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84ACA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951E4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84ACC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951E8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84ACE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951EC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AD00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA951F0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AD20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9577C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AD40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95780, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AD60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95784, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AD80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95788, 0x0, 4, 1, 0x39D1B717),
    /* 0x84ADA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9578C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84ADC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95790, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84ADE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95794, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AE00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95798, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AE20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9579C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AE40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957A0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AE60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957A4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AE80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957A8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AEA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957AC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AEC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957B0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AEE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957B4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AF00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957B8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AF20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957BC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84AF40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957C0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84AF60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957C4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84AF80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957C8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84AFA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957CC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84AFC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957D0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84AFE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957D4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B000 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957D8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957DC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B040 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957E0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B060 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957E4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B080 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957E8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B0A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957EC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B0C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957F0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B0E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA957F4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B100 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95800, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B120 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95804, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95808, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9580C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95810, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B1A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA95814, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B1C0 420 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84B1C0, 0x0, 0, 0, 0x0), // 420 write(s) already zero in the native build
    /* 0x84B1E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A7F4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B200 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A7F8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B220 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A7FC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B240 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A800, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B260 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A804, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B280 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A808, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B2A0 mov ecx, 0xa9a810 ; jmp 0x5a8020 */
    EXEDATA_ROW(ObjectCtor, 0xA9A810, 0x5A8020, 0, 0, 0x0), // mov ecx, 0xa9a810 ; jmp 0x5a8020
    /* 0x84B2B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8A4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B2D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8A8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B2F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8AC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B310 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8B0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B330 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8B4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B350 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8B8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B370 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9A8BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B390 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD7C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B3B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD80, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B3D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD84, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B3F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD88, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B410 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD8C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B430 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD90, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD98, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B470 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AD9C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B490 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADA0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B4B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADA4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B4D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADA8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B4F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADAC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B510 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADE8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B530 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADEC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B550 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADF0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B570 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADF4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B590 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADF8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B5B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9ADFC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B5D0 mov ecx, 0xa9ae00 ; call 0x49e620 */
    EXEDATA_ROW(ObjectCtor, 0xA9AE00, 0x49E620, 0, 0, 0x0), // mov ecx, 0xa9ae00 ; call 0x49e620
    /* 0x84B5F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AE70, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B610 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AE74, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B630 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AE78, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B650 mov ecx, 0xa9ae80 ; call 0x4a9470 */
    EXEDATA_ROW(ObjectCtor, 0xA9AE80, 0x4A9470, 0, 0, 0x0), // mov ecx, 0xa9ae80 ; call 0x4a9470
    /* 0x84B670 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF3C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B690 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF40, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B6B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF44, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B6D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF48, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B6F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF4C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B710 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF50, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B730 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF54, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B750 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF58, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B770 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF5C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B790 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF60, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B7B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF64, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B7D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF68, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B7F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF70, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B810 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF74, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B830 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF78, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B850 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF7C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B870 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF80, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B890 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF84, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B8B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF88, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B8D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF8C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B8F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF90, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B910 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF94, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B930 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF98, 0x0, 4, 1, 0x3B83126E),
    /* 0x84B950 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AF9C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84B970 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFA0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84B990 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFA4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84B9B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFA8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84B9D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFAC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84B9F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFB0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84BA10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9AFB4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84BA30 _eh_vector_constructor_iterator(ctor 0x4AC0D0 dtor 0x4B2950 8 x 0xC) */
    EXEDATA_ROW(FillPattern, 0xA9AFC0, 0xC, 4, 8, 0xFFFFFFFF), // element ctor 0x4AC0D0
    /* 0x84BA60 _eh_vector_constructor_iterator(ctor 0x4AC350 dtor 0x4B29A0 8 x 0x10) */
    EXEDATA_ROW(FillPattern, 0xA9B024, 0x10, 4, 8, 0xFFFFFFFF), // element ctor 0x4AC350
    /* 0x84BA90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B098, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BAB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B09C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84BAD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0A0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84BAF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0A4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84BB10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0A8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84BB30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0AC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84BB50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0B4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BB70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BB90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BBB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xA9B0C0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BBD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xAAE94C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BBF0 mov ecx, 0xaae950 ; call 0x4c5cb0 */
    EXEDATA_ROW(ObjectCtor, 0xAAE950, 0x4C5CB0, 0, 0, 0x0), // mov ecx, 0xaae950 ; call 0x4c5cb0
    /* 0x84BC10 mov ecx, 0xb1bf58 ; call 0x4c5d10 */
    EXEDATA_ROW(ObjectCtor, 0xB1BF58, 0x4C5D10, 0, 0, 0x0), // mov ecx, 0xb1bf58 ; call 0x4c5d10
    /* 0x84BC30 mov ecx, 0xb1c934 ; call 0x4c5d70 */
    EXEDATA_ROW(ObjectCtor, 0xB1C934, 0x4C5D70, 0, 0, 0x0), // mov ecx, 0xb1c934 ; call 0x4c5d70
    /* 0x84BC50 mov ecx, 0xb1c960 ; call 0x4c5dd0 */
    EXEDATA_ROW(ObjectCtor, 0xB1C960, 0x4C5DD0, 0, 0, 0x0), // mov ecx, 0xb1c960 ; call 0x4c5dd0
    /* 0x84BC70 mov ecx, 0xb1e128 ; call 0x4c5e30 */
    EXEDATA_ROW(ObjectCtor, 0xB1E128, 0x4C5E30, 0, 0, 0x0), // mov ecx, 0xb1e128 ; call 0x4c5e30
    /* 0x84BC90 mov ecx, 0xb1e158 ; call 0x4c5e90 */
    EXEDATA_ROW(ObjectCtor, 0xB1E158, 0x4C5E90, 0, 0, 0x0), // mov ecx, 0xb1e158 ; call 0x4c5e90
    /* 0x84BCB0 mov ecx, 0xb1e958 ; call 0x4c5ef0 */
    EXEDATA_ROW(ObjectCtor, 0xB1E958, 0x4C5EF0, 0, 0, 0x0), // mov ecx, 0xb1e958 ; call 0x4c5ef0
    /* 0x84BCD0 mov ecx, 0xb1f650 ; call 0x4c5f50 */
    EXEDATA_ROW(ObjectCtor, 0xB1F650, 0x4C5F50, 0, 0, 0x0), // mov ecx, 0xb1f650 ; call 0x4c5f50
    /* 0x84BCF0 mov ecx, 0xb478f8 ; call 0x4c67d0 */
    EXEDATA_ROW(ObjectCtor, 0xB478F8, 0x4C67D0, 0, 0, 0x0), // mov ecx, 0xb478f8 ; call 0x4c67d0
    /* 0x84BD10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4DBDC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BD30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4DBE0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BD50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6C0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BD70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6C4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84BD90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6C8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84BDB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6CC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84BDD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6D0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84BDF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E6D4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84BE10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E754, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BE30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E758, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84BE50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E75C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84BE70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E760, 0x0, 4, 1, 0x39D1B717),
    /* 0x84BE90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E764, 0x0, 4, 1, 0x3B83126E),
    /* 0x84BEB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4E768, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84BED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA04, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BEF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA10, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BF10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA14, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84BF30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA18, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84BF50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA1C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84BF70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA20, 0x0, 4, 1, 0x3B83126E),
    /* 0x84BF90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA24, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84BFB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB4EA38, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84BFD0 _eh_vector_constructor_iterator(ctor 0x4CF270 dtor 0x4CF970 2500 x 0x18) */
    EXEDATA_ROW(VectorCtor, 0xB4EA40, 0x4CF270, 24, 2500, 0x0), // _eh_vector_constructor_iterator(ctor 0x4CF270 dtor 0x4CF970 2500 x 0x18)
    /* 0x84C000 360 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84C000, 0x0, 0, 0, 0x0), // 360 write(s) already zero in the native build
    /* 0x84C020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F860, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C040 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F864, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C060 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F868, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C080 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F86C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C0A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F870, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C0C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F874, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C0E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F880, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C100 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F884, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C120 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F888, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F88C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F890, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F894, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C1A0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84C1A0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84C1B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F89C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C1D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F8A0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C1F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F8A4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F8A8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F8AC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB5F8B0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C270 mov ecx, 0xb5f8b8 ; call 0x4d83e0 */
    EXEDATA_ROW(ObjectCtor, 0xB5F8B8, 0x4D83E0, 0, 0, 0x0), // mov ecx, 0xb5f8b8 ; call 0x4d83e0
    /* 0x84C290 mov ecx, 0xb608d0 ; call 0x4ee9a0 */
    EXEDATA_ROW(ObjectCtor, 0xB608D0, 0x4EE9A0, 0, 0, 0x0), // mov ecx, 0xb608d0 ; call 0x4ee9a0
    /* 0x84C2B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61290, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C2D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61294, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C2F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61298, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C310 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6129C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C330 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612A0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C350 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612A4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C370 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612A8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C390 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612AC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C3B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612B0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C3D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612B4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C3F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612B8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C410 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612BC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C430 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612C0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612C4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C470 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612C8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C490 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612CC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C4B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612D0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C4D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612D4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C4F0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84C4F0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84C500 mov ecx, 0xb612d8 ; call 0x4f1750 */
    EXEDATA_ROW(ObjectCtor, 0xB612D8, 0x4F1750, 0, 0, 0x0), // mov ecx, 0xb612d8 ; call 0x4f1750
    /* 0x84C520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB612E8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61300, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61304, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61308, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C5A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6130C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C5C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61310, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C5E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61314, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6136C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61370, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61374, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61378, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6137C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C6A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61380, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C6C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6138C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C6E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61390, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61394, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61398, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6139C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613A0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613A4, 0x0, 4, 1, 0x42A60000),
    /* 0x84C7A0 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84C7A0, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x84C7C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613AC, 0x0, 4, 1, 0x42AA0000),
    /* 0x84C7E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613B0, 0x0, 4, 1, 0xBD8F5C28),
    /* 0x84C800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613B4, 0x0, 4, 1, 0x41100000),
    /* 0x84C820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613B8, 0x0, 4, 1, 0x3E9EB851),
    /* 0x84C840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613BC, 0x0, 4, 1, 0x3ECCCCCE),
    /* 0x84C860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613C0, 0x0, 4, 1, 0x3F4CCCCD),
    /* 0x84C880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613C4, 0x0, 4, 1, 0x3ECCCCCE),
    /* 0x84C8A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613C8, 0x0, 4, 1, 0x3F19999A),
    /* 0x84C8C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB613CC, 0x0, 4, 1, 0x3F000000),
    /* 0x84C8E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C1C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C20, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84C920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C24, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84C940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C28, 0x0, 4, 1, 0x39D1B717),
    /* 0x84C960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C2C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84C980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61C30, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84C9A0 _eh_vector_constructor_iterator(ctor 0x4E3330 dtor 0x4E3340 6 x 0x1C) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84C9A0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x4E3330 dtor 0x4E3340 6 x 0x1C) (element ctor only clears)
    /* 0x84C9D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CE0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84C9F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CE4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84CA10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CE8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84CA30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CEC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84CA50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CF0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84CA70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61CF4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84CA90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D58, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84CAB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D5C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84CAD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D60, 0x0, 4, 1, 0x39D1B717),
    /* 0x84CAF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D64, 0x0, 4, 1, 0x3B83126E),
    /* 0x84CB10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D68, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84CB30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D6C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CB50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB61D74, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CB70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C74, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84CB90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C78, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84CBB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C7C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84CBD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C80, 0x0, 4, 1, 0x3B83126E),
    /* 0x84CBF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C84, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84CC10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C88, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CC30 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84CC30, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84CC40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C8C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CC60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C90, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84CC80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C94, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84CCA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C98, 0x0, 4, 1, 0x39D1B717),
    /* 0x84CCC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62C9C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84CCE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62CA0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84CD00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB62CA8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CD20 mov ecx, 0xb62cb0 ; call 0x4efd30 */
    EXEDATA_ROW(ObjectCtor, 0xB62CB0, 0x4EFD30, 0, 0, 0x0), // mov ecx, 0xb62cb0 ; call 0x4efd30
    /* 0x84CD40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6B96C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CD60 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84CD60, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84CD70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA08, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84CD90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA0C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84CDB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA10, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84CDD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA14, 0x0, 4, 1, 0x39D1B717),
    /* 0x84CDF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA18, 0x0, 4, 1, 0x3B83126E),
    /* 0x84CE10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA1C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84CE30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA20, 0x0, 4, 1, 0x3EB33334),
    /* 0x84CE50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA24, 0x0, 4, 1, 0x40400000),
    /* 0x84CE70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA28, 0x0, 4, 1, 0x3F19999A),
    /* 0x84CE90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA2C, 0x0, 4, 1, 0x40900000),
    /* 0x84CEB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA30, 0x0, 4, 1, 0x3E800000),
    /* 0x84CED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA34, 0x0, 4, 1, 0x40000000),
    /* 0x84CEF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA38, 0x0, 4, 1, 0x3ECCCCCD),
    /* 0x84CF10 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xB6BA3C, 0x0, 4, 1, 0xC0000000),
    /* 0x84CF20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA40, 0x0, 4, 1, 0x3F266666),
    /* 0x84CF40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA44, 0x0, 4, 1, 0x3FC00000),
    /* 0x84CF60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA48, 0x0, 4, 1, 0x3EA66666),
    /* 0x84CF80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA4C, 0x0, 4, 1, 0x40000000),
    /* 0x84CFA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA50, 0x0, 4, 1, 0x3B0DFEA2),
    /* 0x84CFC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA54, 0x0, 4, 1, 0x3C5A740E),
    /* 0x84CFE0 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xB6BA58, 0x0, 4, 1, 0x3F800000),
    /* 0x84CFF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA5C, 0x0, 4, 1, 0x3F000000),
    /* 0x84D010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA60, 0x0, 4, 1, 0x3F800000),
    /* 0x84D030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA64, 0x0, 4, 1, 0x40800000),
    /* 0x84D050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA68, 0x0, 4, 1, 0x3E4CCCCD),
    /* 0x84D070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA6C, 0x0, 4, 1, 0x3DCCCCC8),
    /* 0x84D090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA70, 0x0, 4, 1, 0x3F333334),
    /* 0x84D0B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA74, 0x0, 4, 1, 0x3F333332),
    /* 0x84D0D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA78, 0x0, 4, 1, 0x3ECCCCCE),
    /* 0x84D0F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA7C, 0x0, 4, 1, 0x41100000),
    /* 0x84D110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA80, 0x0, 4, 1, 0x3E4CCCD0),
    /* 0x84D130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA84, 0x0, 4, 1, 0x3ECCCCCE),
    /* 0x84D150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA88, 0x0, 4, 1, 0x3EA3D70A),
    /* 0x84D170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA8C, 0x0, 4, 1, 0x40400000),
    /* 0x84D190 2 row(s) */
    EXEDATA_ROW(StoreBytes, 0x8CC0B8, 0x0, 4, 1, 0xC1000000),
    /* cont. */
    EXEDATA_ROW(StoreBytes, 0x8CC0C0, 0x0, 4, 1, 0xC0A00000),
    /* 0x84D1B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA90, 0x0, 4, 1, 0x40C00000),
    /* 0x84D1D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA94, 0x0, 4, 1, 0x3EB33332),
    /* 0x84D1F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA98, 0x0, 4, 1, 0x40C00000),
    /* 0x84D210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BA9C, 0x0, 4, 1, 0x3EB33334),
    /* 0x84D230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAA0, 0x0, 4, 1, 0x40400000),
    /* 0x84D250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAA4, 0x0, 4, 1, 0x3E4CCCD0),
    /* 0x84D270 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAA8, 0x0, 4, 1, 0x40C00000),
    /* 0x84D290 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAAC, 0x0, 4, 1, 0x3E99999C),
    /* 0x84D2B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAB0, 0x0, 4, 1, 0x41000000),
    /* 0x84D2D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAB4, 0x0, 4, 1, 0x3DB851E8),
    /* 0x84D2F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAB8, 0x0, 4, 1, 0x41D80000),
    /* 0x84D310 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BABC, 0x0, 4, 1, 0x3E4CCCCC),
    /* 0x84D330 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BADC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D350 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAE0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D370 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAE4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D390 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAE8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D3B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAEC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D3D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAF0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D3F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BAF4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D410 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB00, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D430 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB04, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB08, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D470 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB0C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D490 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB10, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D4B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BB14, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D4D0 mov ecx, 0xb6bb18 ; call 0x4f63b0 */
    EXEDATA_ROW(ObjectCtor, 0xB6BB18, 0x4F63B0, 0, 0, 0x0), // mov ecx, 0xb6bb18 ; call 0x4f63b0
    /* 0x84D4F0 mov ecx, 0xb6bbc0 ; call 0x4f63b0 */
    EXEDATA_ROW(ObjectCtor, 0xB6BBC0, 0x4F63B0, 0, 0, 0x0), // mov ecx, 0xb6bbc0 ; call 0x4f63b0
    /* 0x84D510 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC78, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D530 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC7C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D550 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC80, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D570 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC84, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D590 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC88, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D5B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6BC8C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D5D0 mov ecx, 0xb6bc90 ; call 0x507670 */
    EXEDATA_ROW(ObjectCtor, 0xB6BC90, 0x507670, 0, 0, 0x0), // mov ecx, 0xb6bc90 ; call 0x507670
    /* 0x84D5F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EBA0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D610 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC88, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D630 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC8C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D650 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC90, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D670 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC94, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D690 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC98, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D6B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB6EC9C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D6D0 _eh_vector_constructor_iterator(ctor 0x517740 dtor 0x509810 6 x 0x94) */
    EXEDATA_ROW(VectorCtor, 0xB6ECA0, 0x517740, 148, 6, 0x0), // _eh_vector_constructor_iterator(ctor 0x517740 dtor 0x509810 6 x 0x94)
    /* 0x84D700 mov ecx, 0xb6f028 ; call 0x51a450 */
    EXEDATA_ROW(ObjectCtor, 0xB6F028, 0x51A450, 0, 0, 0x0), // mov ecx, 0xb6f028 ; call 0x51a450
    /* 0x84D720 mov ecx, 0xb6fda0 ; call 0x50e6d0 */
    EXEDATA_ROW(ObjectCtor, 0xB6FDA0, 0x50E6D0, 0, 0, 0x0), // mov ecx, 0xb6fda0 ; call 0x50e6d0
    /* 0x84D740 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84D740, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84D750 stores the address 0x8CC958 at 0xB6FE88 */
    EXEDATA_ROW(Unhandled, 0x84D750, 0x0, 0, 0, 0x0), // stores the address 0x8CC958 at 0xB6FE88
    /* 0x84D770 _eh_vector_constructor_iterator(ctor 0x5173F0 dtor 0x50E080 1 x 0x9C) */
    EXEDATA_ROW(VectorCtor, 0xB6FEC0, 0x5173F0, 156, 1, 0x0), // _eh_vector_constructor_iterator(ctor 0x5173F0 dtor 0x50E080 1 x 0x9C)
    /* 0x84D7A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70160, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D7C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70164, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D7E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70168, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7016C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70170, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70174, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7017C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70180, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D8A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70184, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D8C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70188, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D8E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7018C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB70190, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D920 mov ecx, 0xb70198 ; jmp 0x531ee0 */
    EXEDATA_ROW(ObjectCtor, 0xB70198, 0x531EE0, 0, 0, 0x0), // mov ecx, 0xb70198 ; jmp 0x531ee0
    /* 0x84D930 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB714F0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84D950 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB714F4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84D970 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB714F8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84D990 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB714FC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84D9B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71500, 0x0, 4, 1, 0x3B83126E),
    /* 0x84D9D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71504, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84D9F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717D4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DA10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717D8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DA30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717DC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DA50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717E0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DA70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717E4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DA90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717E8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DAB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717EC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DAD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717F0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DAF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717F4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DB10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717F8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DB30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB717FC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DB50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71800, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DB70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7180C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DB90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71810, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DBB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71814, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DBD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71818, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DBF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7181C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DC10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71820, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DC30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71824, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DC50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71828, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DC70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7182C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DC90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71830, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DCB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71834, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DCD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71838, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DCF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A48, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DD10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A4C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DD30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A50, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DD50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A54, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DD70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A58, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DD90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71A5C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DDB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F68, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DDD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F6C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DDF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F70, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DE10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F74, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DE30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F78, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DE50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB71F7C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DE70 mov ecx, 0xb71f80 ; call 0x539da0 */
    EXEDATA_ROW(ObjectCtor, 0xB71F80, 0x539DA0, 0, 0, 0x0), // mov ecx, 0xb71f80 ; call 0x539da0
    /* 0x84DE90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72928, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DEB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7292C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72930, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DEF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72934, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DF10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72938, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DF30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7293C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84DF50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72960, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84DF70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72964, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84DF90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72968, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84DFB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7296C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84DFD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72970, 0x0, 4, 1, 0x3B83126E),
    /* 0x84DFF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72974, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E010 mov ecx, 0xb72978 ; call 0x53cfa0 */
    EXEDATA_ROW(ObjectCtor, 0xB72978, 0x53CFA0, 0, 0, 0x0), // mov ecx, 0xb72978 ; call 0x53cfa0
    /* 0x84E030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C88, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C8C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C90, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C94, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E0B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C98, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E0D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72C9C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E0F0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84E0F0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84E100 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84E100, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84E110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB72CAC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB73440, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB73444, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB73448, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E190 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7344C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E1B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB73450, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E1D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB73454, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E1F0 _eh_vector_constructor_iterator(ctor 0x541D80 dtor 0x53ED60 2 x 0x134) */
    EXEDATA_ROW(VectorCtor, 0xB73458, 0x541D80, 308, 2, 0x0), // _eh_vector_constructor_iterator(ctor 0x541D80 dtor 0x53ED60 2 x 0x134)
    /* 0x84E220 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7421C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E240 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB74220, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E260 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB74224, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E280 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB74228, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E2A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7422C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E2C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB74230, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E2E0 mov ecx, 0xb74240 ; call 0x59aed0 ; args 0x3F800000 */
    EXEDATA_ROW(ObjectCtor, 0xB74240, 0x59AED0, 0, 0, 0x0), // mov ecx, 0xb74240 ; call 0x59aed0 ; args 0x3F800000
    /* 0x84E300 mov ecx, 0xb74288 ; call 0x54f440 */
    EXEDATA_ROW(ObjectCtor, 0xB74288, 0x54F440, 0, 0, 0x0), // mov ecx, 0xb74288 ; call 0x54f440
    /* 0x84E320 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744C8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744CC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744D0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744D4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E3A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744D8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E3C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB744DC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E3E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745A4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745A8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745AC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745B0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745B4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB745B8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E4A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB76858, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E4C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7685C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E4E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB76860, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB76864, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB76868, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7686C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7689C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB768A0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E5A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB768A4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E5C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB768A8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E5E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB768AC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB768B0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB79518, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7951C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB79520, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB79524, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E6A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB79528, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E6C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7952C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E6E0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84E6E0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84E6F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C488, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E710 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C48C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E730 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C490, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E750 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C494, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E770 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C498, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E790 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7C49C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E7B0 mov ecx, 0xb7cb10 ; call 0x7170c0 ; args 0xFF, 0x46, 0x46, 0x46 */
    EXEDATA_ROW(ObjectCtor, 0xB7CB10, 0x7170C0, 0, 0, 0x0), // mov ecx, 0xb7cb10 ; call 0x7170c0 ; args 0xFF, 0x46, 0x46, 0x46
    /* 0x84E7E0 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84E7E0, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x84E7F0 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x84E7F0, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x84E800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CB90, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CB94, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CB98, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CB9C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CBA0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E8A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CBA4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E8C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD80, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84E8E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD84, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84E900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD88, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84E920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD8C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84E940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD90, 0x0, 4, 1, 0x3B83126E),
    /* 0x84E960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB7CD94, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84E980 _eh_vector_constructor_iterator(ctor 0x571920 dtor 0x45B110 2 x 0x190) */
    EXEDATA_ROW(VectorCtor, 0xB7CD98, 0x571920, 400, 2, 0x0), // _eh_vector_constructor_iterator(ctor 0x571920 dtor 0x45B110 2 x 0x190)
    /* 0x84E9B0 _eh_vector_constructor_iterator(ctor 0x564040 dtor 0x563130 14400 x 0x8) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84E9B0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x564040 dtor 0x563130 14400 x 0x8) (element ctor only clears)
    /* 0x84E9E0 _eh_vector_constructor_iterator(ctor 0x563190 dtor 0x5631B0 256 x 0xC) */
    EXEDATA_ROW(VectorCtor, 0xB992B8, 0x563190, 12, 256, 0x0), // _eh_vector_constructor_iterator(ctor 0x563190 dtor 0x5631B0 256 x 0xC)
    /* 0x84EA10 _eh_vector_constructor_iterator(ctor 0x43E3F0 dtor 0x43E1A0 900 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84EA10, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x43E3F0 dtor 0x43E1A0 900 x 0x4) (element ctor only clears)
    /* 0x84EA40 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84EA40, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84EA50 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84EA50, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84EA60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7D4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EA80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7D8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EAA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7DC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EAC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7E0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EAE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7E4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EB00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B7E8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84EB20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B97C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EB40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B980, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EB60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B984, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EB80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B988, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EBA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B98C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EBC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xB9B990, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84EBE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA176C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EC00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA1770, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EC20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA1774, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EC40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA1778, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EC60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA177C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EC80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA1780, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84ECA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18DC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84ECC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18E0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84ECE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18E4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84ED00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18E8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84ED20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18EC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84ED40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA18F0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84ED60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA671C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84ED80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA672C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EDA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA6730, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EDC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA6734, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EDE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA6738, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EE00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA673C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EE20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA6740, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84EE40 mov ecx, 0xba6748 ; call 0x574350 */
    EXEDATA_ROW(ObjectCtor, 0xBA6748, 0x574350, 0, 0, 0x0), // mov ecx, 0xba6748 ; call 0x574350
    /* 0x84EE60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82C0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EE80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82C4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EEA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82C8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EEC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82CC, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EEE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82D0, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EF00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82D4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84EF20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82E4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84EF40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82E8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84EF60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82EC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84EF80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82F0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84EFA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82F4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84EFC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA82F8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84EFE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F000 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86C0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86C4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F040 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86C8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F060 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86CC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F080 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBA86D0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F0A0 _eh_vector_constructor_iterator(ctor 0x463360 dtor 0x43CE70 6 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84F0A0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x463360 dtor 0x43CE70 6 x 0x4) (element ctor only clears)
    /* 0x84F0D0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 64 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84F0D0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 64 x 0x4) (element ctor only clears)
    /* 0x84F100 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1E4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F120 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1E8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1EC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1F0, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1F4, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F1A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB1F8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F1C0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 6 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84F1C0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 6 x 0x4) (element ctor only clears)
    /* 0x84F1F0 mov ecx, 0xbab22c ; call 0x58fda0 */
    EXEDATA_ROW(ObjectCtor, 0xBAB22C, 0x58FDA0, 0, 0, 0x0), // mov ecx, 0xbab22c ; call 0x58fda0
    /* 0x84F210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB344, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB348, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB34C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F270 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB350, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F290 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB354, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F2B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAB358, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F2D0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 7 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x84F2D0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 7 x 0x4) (element ctor only clears)
    /* 0x84F300 mov ecx, 0xbab380 ; call 0x591260 */
    EXEDATA_ROW(ObjectCtor, 0xBAB380, 0x591260, 0, 0, 0x0), // mov ecx, 0xbab380 ; call 0x591260
    /* 0x84F320 _eh_vector_constructor_iterator(ctor 0x590E20 dtor 0x590E70 128 x 0x1C) */
    EXEDATA_ROW(VectorCtor, 0xBAD3F8, 0x590E20, 28, 128, 0x0), // _eh_vector_constructor_iterator(ctor 0x590E20 dtor 0x590E70 128 x 0x1C)
    /* 0x84F350 _eh_vector_constructor_iterator(ctor 0x590EC0 dtor 0x590ED0 256 x 0x14) */
    EXEDATA_ROW(VectorCtor, 0xBAE1F8, 0x590EC0, 20, 256, 0x0), // _eh_vector_constructor_iterator(ctor 0x590EC0 dtor 0x590ED0 256 x 0x14)
    /* 0x84F380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF5F8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F3A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF5FC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F3C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF600, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F3E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF604, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF608, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF60C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF610, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF614, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF618, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F4A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF61C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F4C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF620, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F4E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF624, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF628, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF62C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF630, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF634, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF638, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F5A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF63C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F5C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF640, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F5E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF644, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF648, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF64C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF650, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF654, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF658, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F6A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF65C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F6C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF660, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F6E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF664, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF668, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBAF66C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F740 mov ecx, 0xbaf670 ; call 0x5984c0 */
    EXEDATA_ROW(ObjectCtor, 0xBAF670, 0x5984C0, 0, 0, 0x0), // mov ecx, 0xbaf670 ; call 0x5984c0
    /* 0x84F760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DCC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DD0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F7A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DD4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F7C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DD8, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F7E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DDC, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DE0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DE8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DEC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DF0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DF4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F8A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DF8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F8C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB3DFC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F8E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4200, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4204, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4208, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84F940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB420C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84F960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4210, 0x0, 4, 1, 0x3B83126E),
    /* 0x84F980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4214, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84F9A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB421C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84F9C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4220, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84F9E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4224, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FA00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4228, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FA20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB422C, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FA40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4230, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FA60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB423C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FA80 mov ecx, 0xbb4240 ; call 0x59e620 */
    EXEDATA_ROW(ObjectCtor, 0xBB4240, 0x59E620, 0, 0, 0x0), // mov ecx, 0xbb4240 ; call 0x59e620
    /* 0x84FAA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A40, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FAC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A44, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FAE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A48, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FB00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A4C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FB20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A50, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FB40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A54, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FB60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A58, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FB80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A5C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FBA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A60, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FBC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A64, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FBE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A68, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FC00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A6C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FC20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A74, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FC40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A78, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FC60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A7C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FC80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A80, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FCA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A84, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FCC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB4A88, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FCE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7C90, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FD00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7C98, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FD20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7C9C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FD40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7CA0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FD60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7CA4, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FD80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7CA8, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FDA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBB7CAC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FDC0 mov ecx, 0xbb7cb0 ; call 0x5a3e20 */
    EXEDATA_ROW(ObjectCtor, 0xBB7CB0, 0x5A3E20, 0, 0, 0x0), // mov ecx, 0xbb7cb0 ; call 0x5a3e20
    /* 0x84FDE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBBC8C4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FE00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC128C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FE20 mov ecx, 0xbc1290 ; call 0x5a7570 */
    EXEDATA_ROW(ObjectCtor, 0xBC1290, 0x5A7570, 0, 0, 0x0), // mov ecx, 0xbc1290 ; call 0x5a7570
    /* 0x84FE40 mov ecx, 0xbc12c0 ; call 0x532290 */
    EXEDATA_ROW(ObjectCtor, 0xBC12C0, 0x532290, 0, 0, 0x0), // mov ecx, 0xbc12c0 ; call 0x532290
    /* 0x84FE60 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x84FE60, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x84FE70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C60, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FE90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C64, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FEB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C68, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C6C, 0x0, 4, 1, 0x39D1B717),
    /* 0x84FEF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C70, 0x0, 4, 1, 0x3B83126E),
    /* 0x84FF10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1C74, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x84FF30 2 row(s) */
    EXEDATA_ROW(ZeroFill, 0xBC1C78, 0x0, 40, 1, 0x0),
    /* cont. */
    EXEDATA_ROW(ZeroFill, 0xBC1CA0, 0x0, 72, 1, 0x0),
    /* 0x84FF60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC1CF0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FF80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4008, 0x0, 4, 1, 0x39A3D70A),
    /* 0x84FFA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC400C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x84FFC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4010, 0x0, 4, 1, 0x41DE38E4),
    /* 0x84FFE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4014, 0x0, 4, 1, 0x39D1B717),
    /* 0x850000 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4018, 0x0, 4, 1, 0x3B83126E),
    /* 0x850020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC401C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850040 mov ecx, 0xbc4020 ; call 0x4cde70 */
    EXEDATA_ROW(ObjectCtor, 0xBC4020, 0x4CDE70, 0, 0, 0x0), // mov ecx, 0xbc4020 ; call 0x4cde70
    /* 0x850060 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC405C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850080 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4060, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8500A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4064, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8500C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4068, 0x0, 4, 1, 0x39D1B717),
    /* 0x8500E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC406C, 0x0, 4, 1, 0x3B83126E),
    /* 0x850100 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC4070, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850120 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC408C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40C0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40C4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8501A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40C8, 0x0, 4, 1, 0x39D1B717),
    /* 0x8501C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40CC, 0x0, 4, 1, 0x3B83126E),
    /* 0x8501E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBC40D0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850200 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00E0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850220 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00E4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850240 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00E8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850260 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00EC, 0x0, 4, 1, 0x39D1B717),
    /* 0x850280 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00F0, 0x0, 4, 1, 0x3B83126E),
    /* 0x8502A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00F4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8502C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD00FC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8502E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0100, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850300 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0104, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850320 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0108, 0x0, 4, 1, 0x39D1B717),
    /* 0x850340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD010C, 0x0, 4, 1, 0x3B83126E),
    /* 0x850360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0110, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0118, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8503A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD011C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8503C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0120, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8503E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0124, 0x0, 4, 1, 0x39D1B717),
    /* 0x850400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0128, 0x0, 4, 1, 0x3B83126E),
    /* 0x850420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD012C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0130, 0x0, 4, 1, 0x3FBFFFFF),
    /* 0x850460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0134, 0x0, 4, 1, 0x40800000),
    /* 0x850480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0138, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8504A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD013C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8504C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0140, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8504E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0144, 0x0, 4, 1, 0x39D1B717),
    /* 0x850500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0148, 0x0, 4, 1, 0x3B83126E),
    /* 0x850520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD014C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xBD0150, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B84, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B88, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8505A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B8C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8505C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B90, 0x0, 4, 1, 0x39D1B717),
    /* 0x8505E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B94, 0x0, 4, 1, 0x3B83126E),
    /* 0x850600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B98, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02B9C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BA0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BA4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BA8, 0x0, 4, 1, 0x39D1B717),
    /* 0x8506A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BAC, 0x0, 4, 1, 0x3B83126E),
    /* 0x8506C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BB0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8506E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BB4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BB8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BBC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BC0, 0x0, 4, 1, 0x39D1B717),
    /* 0x850760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BC4, 0x0, 4, 1, 0x3B83126E),
    /* 0x850780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02BC8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8507A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02C10, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8507C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02C24, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8507E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02D34, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DC4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DC8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DCC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DD0, 0x0, 4, 1, 0x39D1B717),
    /* 0x850880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DD4, 0x0, 4, 1, 0x3B83126E),
    /* 0x8508A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC02DD8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8508C0 4 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x8508C0, 0x0, 0, 0, 0x0), // 4 write(s) already zero in the native build
    /* 0x8508F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC03A40, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850910 mov ecx, 0xc03a44 ; call 0x7170c0 ; args 0xFF, 0xFF, 0xFF, 0xFF */
    EXEDATA_ROW(ObjectCtor, 0xC03A44, 0x7170C0, 0, 0, 0x0), // mov ecx, 0xc03a44 ; call 0x7170c0 ; args 0xFF, 0xFF, 0xFF, 0xFF
    /* 0x850940 _eh_vector_constructor_iterator(ctor 0x5DB2B0 dtor 0x5DB2C0 256 x 0x54) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x850940, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x5DB2B0 dtor 0x5DB2C0 256 x 0x54) (element ctor only clears)
    /* 0x850970 _eh_vector_constructor_iterator(ctor 0x5DB290 dtor 0x5DB2A0 40 x 0x14) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x850970, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x5DB290 dtor 0x5DB2A0 40 x 0x14) (element ctor only clears)
    /* 0x8509A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09178, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8509C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0917C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8509E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09180, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850A00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09184, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850A20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09188, 0x0, 4, 1, 0x39D1B717),
    /* 0x850A40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0918C, 0x0, 4, 1, 0x3B83126E),
    /* 0x850A60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09190, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850A80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09194, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850AA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09198, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850AC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0919C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850AE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091A0, 0x0, 4, 1, 0x39D1B717),
    /* 0x850B00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091A4, 0x0, 4, 1, 0x3B83126E),
    /* 0x850B20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091A8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850B40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091AC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850B60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091B0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850B80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091B4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850BA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091B8, 0x0, 4, 1, 0x39D1B717),
    /* 0x850BC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091BC, 0x0, 4, 1, 0x3B83126E),
    /* 0x850BE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091C0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850C00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091C4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850C20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091C8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850C40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091CC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850C60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091D0, 0x0, 4, 1, 0x39D1B717),
    /* 0x850C80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091D4, 0x0, 4, 1, 0x3B83126E),
    /* 0x850CA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091D8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850CC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC091EC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850CE0 _eh_vector_constructor_iterator(ctor 0x5DE520 dtor 0x5DE540 10 x 0x10) */
    EXEDATA_ROW(FillPattern, 0xC091F0, 0x10, 1, 10, 0xFF), // element ctor 0x5DE520
    /* 0x850D10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09290, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850D30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09294, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850D50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09298, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850D70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0929C, 0x0, 4, 1, 0x39D1B717),
    /* 0x850D90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC092A0, 0x0, 4, 1, 0x3B83126E),
    /* 0x850DB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC092A4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850DD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09828, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850DF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0982C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850E10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09830, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850E30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09834, 0x0, 4, 1, 0x39D1B717),
    /* 0x850E50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09838, 0x0, 4, 1, 0x3B83126E),
    /* 0x850E70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0983C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850E90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09844, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850EB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09848, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850ED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0984C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850EF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09850, 0x0, 4, 1, 0x39D1B717),
    /* 0x850F10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09854, 0x0, 4, 1, 0x3B83126E),
    /* 0x850F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09858, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x850F50 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x850F50, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x850F60 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x850F60, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x850F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0987C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x850F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09880, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x850FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09884, 0x0, 4, 1, 0x41DE38E4),
    /* 0x850FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09888, 0x0, 4, 1, 0x39D1B717),
    /* 0x850FF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0988C, 0x0, 4, 1, 0x3B83126E),
    /* 0x851010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09890, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0989C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098A0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098A4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098A8, 0x0, 4, 1, 0x39D1B717),
    /* 0x8510B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098AC, 0x0, 4, 1, 0x3B83126E),
    /* 0x8510D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098B0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8510F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x851170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x851190 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8511B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098F0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8511D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098F4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8511F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098F8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC098FC, 0x0, 4, 1, 0x39D1B717),
    /* 0x851230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09900, 0x0, 4, 1, 0x3B83126E),
    /* 0x851250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC09904, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851270 mov ecx, 0xc09908 ; call 0x5f6450 */
    EXEDATA_ROW(ObjectCtor, 0xC09908, 0x5F6450, 0, 0, 0x0), // mov ecx, 0xc09908 ; call 0x5f6450
    /* 0x851290 mov ecx, 0xc0990c ; call 0x5f6480 */
    EXEDATA_ROW(ObjectCtor, 0xC0990C, 0x5F6480, 0, 0, 0x0), // mov ecx, 0xc0990c ; call 0x5f6480
    /* 0x8512B0 mov ecx, 0xc09910 ; call 0x5f64b0 */
    EXEDATA_ROW(ObjectCtor, 0xC09910, 0x5F64B0, 0, 0, 0x0), // mov ecx, 0xc09910 ; call 0x5f64b0
    /* 0x8512D0 mov ecx, 0xc09914 ; call 0x5f64e0 */
    EXEDATA_ROW(ObjectCtor, 0xC09914, 0x5F64E0, 0, 0, 0x0), // mov ecx, 0xc09914 ; call 0x5f64e0
    /* 0x8512F0 mov ecx, 0xc09918 ; call 0x5f6510 */
    EXEDATA_ROW(ObjectCtor, 0xC09918, 0x5F6510, 0, 0, 0x0), // mov ecx, 0xc09918 ; call 0x5f6510
    /* 0x851310 mov ecx, 0xc0991c ; call 0x5f6540 */
    EXEDATA_ROW(ObjectCtor, 0xC0991C, 0x5F6540, 0, 0, 0x0), // mov ecx, 0xc0991c ; call 0x5f6540
    /* 0x851330 _eh_vector_constructor_iterator(ctor 0x5FC150 dtor 0x5FC190 8 x 0x2D4) */
    EXEDATA_ROW(VectorCtor, 0xC09920, 0x5FC150, 724, 8, 0x0), // _eh_vector_constructor_iterator(ctor 0x5FC150 dtor 0x5FC190 8 x 0x2D4)
    /* 0x851360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFC4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFC8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8513A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFCC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8513C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFD0, 0x0, 4, 1, 0x39D1B717),
    /* 0x8513E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFD4, 0x0, 4, 1, 0x3B83126E),
    /* 0x851400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0AFD8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B03C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B040, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B044, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B048, 0x0, 4, 1, 0x39D1B717),
    /* 0x8514A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B04C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8514C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B050, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8514E0 mov ecx, 0xc0b058 ; call 0x6023a0 */
    EXEDATA_ROW(ObjectCtor, 0xC0B058, 0x6023A0, 0, 0, 0x0), // mov ecx, 0xc0b058 ; call 0x6023a0
    /* 0x851500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x851580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x8515A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8515C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1D0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8515E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1D4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1D8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1DC, 0x0, 4, 1, 0x39D1B717),
    /* 0x851640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1E0, 0x0, 4, 1, 0x3B83126E),
    /* 0x851660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0B1E4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851680 _eh_vector_constructor_iterator(ctor 0x608330 dtor 0x6082E0 128 x 0x14) */
    EXEDATA_ROW(FillPattern, 0xC0B1E8, 0x14, 4, 128, 0xFFFFFFFF), // element ctor 0x608330
    /* cont. */
    EXEDATA_ROW(FillPattern, 0xC0B1F8, 0x14, 4, 128, 0x3), // element ctor 0x608330
    /* 0x8516B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BBF0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8516D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BBF4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8516F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BBF8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851710 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BBFC, 0x0, 4, 1, 0x39D1B717),
    /* 0x851730 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC00, 0x0, 4, 1, 0x3B83126E),
    /* 0x851750 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC04, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851770 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC18, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851790 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC1C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8517B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC20, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8517D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC24, 0x0, 4, 1, 0x39D1B717),
    /* 0x8517F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC28, 0x0, 4, 1, 0x3B83126E),
    /* 0x851810 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0BC2C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851830 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E97C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851850 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E980, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851870 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E984, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851890 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E988, 0x0, 4, 1, 0x39D1B717),
    /* 0x8518B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E98C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8518D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0E990, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8518F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCC0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851910 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCC4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851930 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCC8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851950 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCCC, 0x0, 4, 1, 0x39D1B717),
    /* 0x851970 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCD0, 0x0, 4, 1, 0x3B83126E),
    /* 0x851990 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC0FCD4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8519B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC10288, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8519D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1028C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8519F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC10290, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851A10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC10294, 0x0, 4, 1, 0x39D1B717),
    /* 0x851A30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC10298, 0x0, 4, 1, 0x3B83126E),
    /* 0x851A50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1029C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851A70 mov ecx, 0xc10820 ; call 0x617330 */
    EXEDATA_ROW(ObjectCtor, 0xC10820, 0x617330, 0, 0, 0x0), // mov ecx, 0xc10820 ; call 0x617330
    /* 0x851A90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC15430, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851AB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC15434, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851AD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC15438, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851AF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1543C, 0x0, 4, 1, 0x39D1B717),
    /* 0x851B10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC15440, 0x0, 4, 1, 0x3B83126E),
    /* 0x851B30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC15444, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851B50 mov ecx, 0xc15448 ; call 0x617fc0 */
    EXEDATA_ROW(ObjectCtor, 0xC15448, 0x617FC0, 0, 0, 0x0), // mov ecx, 0xc15448 ; call 0x617fc0
    /* 0x851B70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F00, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851B90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F04, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851BB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F08, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851BD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F0C, 0x0, 4, 1, 0x39D1B717),
    /* 0x851BF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F10, 0x0, 4, 1, 0x3B83126E),
    /* 0x851C10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC16F14, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851C30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1701C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851C50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17020, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851C70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17024, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851C90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17028, 0x0, 4, 1, 0x39D1B717),
    /* 0x851CB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1702C, 0x0, 4, 1, 0x3B83126E),
    /* 0x851CD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17030, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851CF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17088, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851D10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1708C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851D30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17090, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851D50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17094, 0x0, 4, 1, 0x39D1B717),
    /* 0x851D70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC17098, 0x0, 4, 1, 0x3B83126E),
    /* 0x851D90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1709C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851DB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170A0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851DD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170A4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851DF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170A8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851E10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170AC, 0x0, 4, 1, 0x39D1B717),
    /* 0x851E30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170B0, 0x0, 4, 1, 0x3B83126E),
    /* 0x851E50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170B4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851E70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851E90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851EB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851ED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x851EF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x851F10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC170CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x851F50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x851F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x851F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x851FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x851FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC177CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x851FF0 mov ecx, 0xc17824 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC17824, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc17824 ; call 0x40fb60
    /* 0x852010 mov ecx, 0xc17854 ; jmp 0x40f030 */
    EXEDATA_ROW(ObjectCtor, 0xC17854, 0x40F030, 0, 0, 0x0), // mov ecx, 0xc17854 ; jmp 0x40f030
    /* 0x852020 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178D8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852040 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178DC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852060 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178E0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852080 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178E4, 0x0, 4, 1, 0x39D1B717),
    /* 0x8520A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178E8, 0x0, 4, 1, 0x3B83126E),
    /* 0x8520C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC178EC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8520E0 _eh_vector_constructor_iterator(ctor 0x632BD0 dtor 0x6389F0 64 x 0x40) */
    EXEDATA_ROW(VectorCtor, 0xC178F0, 0x632BD0, 64, 64, 0x0), // _eh_vector_constructor_iterator(ctor 0x632BD0 dtor 0x6389F0 64 x 0x40)
    /* 0x852110 _eh_vector_constructor_iterator(ctor 0x62EC40 dtor 0x62EC50 48 x 0x10) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x852110, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x62EC40 dtor 0x62EC50 48 x 0x10) (element ctor only clears)
    /* 0x852140 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18BF0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852160 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18BF4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18BF8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8521A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18BFC, 0x0, 4, 1, 0x39D1B717),
    /* 0x8521C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C00, 0x0, 4, 1, 0x3B83126E),
    /* 0x8521E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C04, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852200 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C08, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852220 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C0C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852240 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C10, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852260 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C14, 0x0, 4, 1, 0x39D1B717),
    /* 0x852280 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C18, 0x0, 4, 1, 0x3B83126E),
    /* 0x8522A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C1C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8522C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C24, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8522E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C28, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852300 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C2C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852320 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C30, 0x0, 4, 1, 0x39D1B717),
    /* 0x852340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C34, 0x0, 4, 1, 0x3B83126E),
    /* 0x852360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C38, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C84, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8523A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C88, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8523C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C8C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8523E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C90, 0x0, 4, 1, 0x39D1B717),
    /* 0x852400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C94, 0x0, 4, 1, 0x3B83126E),
    /* 0x852420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C98, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18C9C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852460 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CA0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CA4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8524A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CA8, 0x0, 4, 1, 0x39D1B717),
    /* 0x8524C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CAC, 0x0, 4, 1, 0x3B83126E),
    /* 0x8524E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CB0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CB4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CB8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852540 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CBC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852560 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CC0, 0x0, 4, 1, 0x39D1B717),
    /* 0x852580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CC4, 0x0, 4, 1, 0x3B83126E),
    /* 0x8525A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CC8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8525C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CCC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8525E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CD0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CD4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CD8, 0x0, 4, 1, 0x39D1B717),
    /* 0x852640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CDC, 0x0, 4, 1, 0x3B83126E),
    /* 0x852660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18CE0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852680 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC18CE4, 0x0, 4, 1, 0x42700000),
    /* 0x852690 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC18CE8, 0x0, 4, 1, 0x42700000),
    /* 0x8526A0 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC18CEC, 0x0, 4, 1, 0x42700000),
    /* 0x8526B0 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC18CF0, 0x0, 4, 1, 0x3F800000),
    /* 0x8526C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D0C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8526E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D10, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D14, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D18, 0x0, 4, 1, 0x39D1B717),
    /* 0x852740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D1C, 0x0, 4, 1, 0x3B83126E),
    /* 0x852760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D20, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D30, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8527A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D34, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8527C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D38, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8527E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D3C, 0x0, 4, 1, 0x39D1B717),
    /* 0x852800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D40, 0x0, 4, 1, 0x3B83126E),
    /* 0x852820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18D44, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852840 fcos of a non-zero value */
    EXEDATA_ROW(Unhandled, 0x852840, 0x0, 0, 0, 0x0), // fcos of a non-zero value
    /* 0x852850 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x852850, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x852860 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC18DB4, 0x0, 4, 1, 0x3F000000),
    /* 0x852870 mov ecx, 0xc18db8 ; call 0x66d440 */
    EXEDATA_ROW(ObjectCtor, 0xC18DB8, 0x66D440, 0, 0, 0x0), // mov ecx, 0xc18db8 ; call 0x66d440
    /* 0x852890 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F5C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8528B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F60, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8528D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F64, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8528F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F68, 0x0, 4, 1, 0x39D1B717),
    /* 0x852910 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F6C, 0x0, 4, 1, 0x3B83126E),
    /* 0x852930 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F70, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852950 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F80, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852970 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F84, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852990 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F88, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8529B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F8C, 0x0, 4, 1, 0x39D1B717),
    /* 0x8529D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F90, 0x0, 4, 1, 0x3B83126E),
    /* 0x8529F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC18F94, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852A10 mov ecx, 0xc19518 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC19518, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc19518 ; call 0x40fb60
    /* 0x852A30 mov ecx, 0xc19548 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC19548, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc19548 ; call 0x40fb60
    /* 0x852A50 mov ecx, 0xc19578 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC19578, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc19578 ; call 0x40fb60
    /* 0x852A70 mov ecx, 0xc195a8 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC195A8, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc195a8 ; call 0x40fb60
    /* 0x852A90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19634, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852AB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19638, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852AD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1963C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852AF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19640, 0x0, 4, 1, 0x39D1B717),
    /* 0x852B10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19644, 0x0, 4, 1, 0x3B83126E),
    /* 0x852B30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19648, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852B50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1964C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852B70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19650, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852B90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19654, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852BB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19658, 0x0, 4, 1, 0x39D1B717),
    /* 0x852BD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1965C, 0x0, 4, 1, 0x3B83126E),
    /* 0x852BF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19660, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852C10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19688, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852C30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1968C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852C50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19690, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852C70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19694, 0x0, 4, 1, 0x39D1B717),
    /* 0x852C90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19698, 0x0, 4, 1, 0x3B83126E),
    /* 0x852CB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1969C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852CD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196A0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852CF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196A4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852D10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196A8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852D30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196AC, 0x0, 4, 1, 0x39D1B717),
    /* 0x852D50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196B0, 0x0, 4, 1, 0x3B83126E),
    /* 0x852D70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196B4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852D90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852DB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852DD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852DF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x852E10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x852E30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852E50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196D0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852E70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196D4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852E90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196D8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852EB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196DC, 0x0, 4, 1, 0x39D1B717),
    /* 0x852ED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196E0, 0x0, 4, 1, 0x3B83126E),
    /* 0x852EF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC196E4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852F10 mov ecx, 0xc196e8 ; call 0x694850 */
    EXEDATA_ROW(ObjectCtor, 0xC196E8, 0x694850, 0, 0, 0x0), // mov ecx, 0xc196e8 ; call 0x694850
    /* 0x852F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1976C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x852F50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19770, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x852F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19774, 0x0, 4, 1, 0x41DE38E4),
    /* 0x852F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19778, 0x0, 4, 1, 0x39D1B717),
    /* 0x852FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1977C, 0x0, 4, 1, 0x3B83126E),
    /* 0x852FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19780, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x852FF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19784, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19788, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1978C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19790, 0x0, 4, 1, 0x39D1B717),
    /* 0x853070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19794, 0x0, 4, 1, 0x3B83126E),
    /* 0x853090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC19798, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8530B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197B0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8530D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197B4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8530F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197B8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197BC, 0x0, 4, 1, 0x39D1B717),
    /* 0x853130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197C0, 0x0, 4, 1, 0x3B83126E),
    /* 0x853150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC197C4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853170 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x853170, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x853180 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A300, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8531A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A304, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8531C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A308, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8531E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A30C, 0x0, 4, 1, 0x39D1B717),
    /* 0x853200 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A310, 0x0, 4, 1, 0x3B83126E),
    /* 0x853220 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A314, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853240 1 row(s) */
    EXEDATA_ROW(ZeroFill, 0xC1A43C, 0x0, 24, 1, 0x0),
    /* 0x853260 1 row(s) */
    EXEDATA_ROW(ZeroFill, 0xC1A45C, 0x0, 120, 1, 0x0),
    /* 0x853280 1 row(s) */
    EXEDATA_ROW(ZeroFill, 0xC1A4DC, 0x0, 120, 1, 0x0),
    /* 0x8532A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A558, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8532C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A55C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8532E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A560, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853300 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A564, 0x0, 4, 1, 0x39D1B717),
    /* 0x853320 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A568, 0x0, 4, 1, 0x3B83126E),
    /* 0x853340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1A56C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1AEB0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1AEB4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8533A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B328, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8533C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B32C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8533E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B330, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B334, 0x0, 4, 1, 0x39D1B717),
    /* 0x853420 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B338, 0x0, 4, 1, 0x3B83126E),
    /* 0x853440 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1B33C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853460 mov ecx, 0xc1b340 ; call 0x6a00f0 */
    EXEDATA_ROW(ObjectCtor, 0xC1B340, 0x6A00F0, 0, 0, 0x0), // mov ecx, 0xc1b340 ; call 0x6a00f0
    /* 0x853480 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFDC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8534A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFE0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8534C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFE4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8534E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFE8, 0x0, 4, 1, 0x39D1B717),
    /* 0x853500 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFEC, 0x0, 4, 1, 0x3B83126E),
    /* 0x853520 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1BFF0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853540 2 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C20C, 0x0, 4, 1, 0x408B020C),
    /* cont. */
    EXEDATA_ROW(StoreFloat, 0xC1C210, 0x0, 4, 1, 0x3F2C8B44),
    /* 0x853580 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C808, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8535A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C80C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8535C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C810, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8535E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C814, 0x0, 4, 1, 0x39D1B717),
    /* 0x853600 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C818, 0x0, 4, 1, 0x3B83126E),
    /* 0x853620 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C81C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853640 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C824, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853660 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C828, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853680 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C82C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8536A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C830, 0x0, 4, 1, 0x39D1B717),
    /* 0x8536C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C834, 0x0, 4, 1, 0x3B83126E),
    /* 0x8536E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C838, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C840, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C844, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C848, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C84C, 0x0, 4, 1, 0x39D1B717),
    /* 0x853780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C850, 0x0, 4, 1, 0x3B83126E),
    /* 0x8537A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C854, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8537C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C878, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8537E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C87C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C880, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C884, 0x0, 4, 1, 0x39D1B717),
    /* 0x853840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C888, 0x0, 4, 1, 0x3B83126E),
    /* 0x853860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C88C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853880 mov ecx, 0xc1c890 ; call 0x6c2740 */
    EXEDATA_ROW(ObjectCtor, 0xC1C890, 0x6C2740, 0, 0, 0x0), // mov ecx, 0xc1c890 ; call 0x6c2740
    /* 0x8538A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C974, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8538C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C978, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8538E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C97C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C980, 0x0, 4, 1, 0x39D1B717),
    /* 0x853920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C984, 0x0, 4, 1, 0x3B83126E),
    /* 0x853940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1C988, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAC0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAC4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8539A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAC8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8539C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CACC, 0x0, 4, 1, 0x39D1B717),
    /* 0x8539E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAD0, 0x0, 4, 1, 0x3B83126E),
    /* 0x853A00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAD4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853A20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAE4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853A40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAE8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853A60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAEC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853A80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAF0, 0x0, 4, 1, 0x39D1B717),
    /* 0x853AA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAF4, 0x0, 4, 1, 0x3B83126E),
    /* 0x853AC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CAF8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853AE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB00, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853B00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB04, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853B20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB08, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853B40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB0C, 0x0, 4, 1, 0x39D1B717),
    /* 0x853B60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB10, 0x0, 4, 1, 0x3B83126E),
    /* 0x853B80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB14, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853BA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB18, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853BC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB1C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853BE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB20, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853C00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB24, 0x0, 4, 1, 0x39D1B717),
    /* 0x853C20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB28, 0x0, 4, 1, 0x3B83126E),
    /* 0x853C40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB2C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853C60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB44, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853C80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB48, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853CA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB4C, 0x0, 4, 1, 0x39D1B717),
    /* 0x853CC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB50, 0x0, 4, 1, 0x3B83126E),
    /* 0x853CE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CB54, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853D00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC5C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853D20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC60, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853D40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC64, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853D60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC68, 0x0, 4, 1, 0x39D1B717),
    /* 0x853D80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC6C, 0x0, 4, 1, 0x3B83126E),
    /* 0x853DA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1CC70, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853DC0 _eh_vector_constructor_iterator(ctor 0x40FB60 dtor 0x40F700 4 x 0x30) */
    EXEDATA_ROW(VectorCtor, 0xC1CC78, 0x40FB60, 48, 4, 0x0), // _eh_vector_constructor_iterator(ctor 0x40FB60 dtor 0x40F700 4 x 0x30)
    /* 0x853DF0 mov ecx, 0xc1cd38 ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC1CD38, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc1cd38 ; call 0x40fb60
    /* 0x853E10 mov ecx, 0xc1cd68 ; jmp 0x40f030 */
    EXEDATA_ROW(ObjectCtor, 0xC1CD68, 0x40F030, 0, 0, 0x0), // mov ecx, 0xc1cd68 ; jmp 0x40f030
    /* 0x853E20 _eh_vector_constructor_iterator(ctor 0x5BD420 dtor 0x5B9E30 30 x 0x94) */
    EXEDATA_ROW(VectorCtor, 0xC1CDC0, 0x5BD420, 148, 30, 0x0), // _eh_vector_constructor_iterator(ctor 0x5BD420 dtor 0x5B9E30 30 x 0x94)
    /* 0x853E50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF18, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853E70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF1C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853E90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF20, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853EB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF24, 0x0, 4, 1, 0x39D1B717),
    /* 0x853ED0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF28, 0x0, 4, 1, 0x3B83126E),
    /* 0x853EF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC1DF2C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853F10 mov ecx, 0xc1df30 ; call 0x6e3eb0 */
    EXEDATA_ROW(ObjectCtor, 0xC1DF30, 0x6E3EB0, 0, 0, 0x0), // mov ecx, 0xc1df30 ; call 0x6e3eb0
    /* 0x853F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC228F4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x853F50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC228F8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x853F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC228FC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x853F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC22900, 0x0, 4, 1, 0x39D1B717),
    /* 0x853FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC22904, 0x0, 4, 1, 0x3B83126E),
    /* 0x853FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC22908, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x853FF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279B4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279B8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279BC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279C0, 0x0, 4, 1, 0x39D1B717),
    /* 0x854070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279C4, 0x0, 4, 1, 0x3B83126E),
    /* 0x854090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279C8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8540B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC279CC, 0x0, 4, 1, 0x3C03126F),
    /* 0x8540D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B950, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8540F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B954, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B958, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B95C, 0x0, 4, 1, 0x39D1B717),
    /* 0x854150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B960, 0x0, 4, 1, 0x3B83126E),
    /* 0x854170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B964, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854190 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B978, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8541B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B97C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8541D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B980, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8541F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B984, 0x0, 4, 1, 0x39D1B717),
    /* 0x854210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B988, 0x0, 4, 1, 0x3B83126E),
    /* 0x854230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B98C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B990, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854270 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B994, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854290 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B998, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8542B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B99C, 0x0, 4, 1, 0x39D1B717),
    /* 0x8542D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9A0, 0x0, 4, 1, 0x3B83126E),
    /* 0x8542F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9A4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854310 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9AC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854330 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9B0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854350 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9B4, 0x0, 4, 1, 0x39D1B717),
    /* 0x854370 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9B8, 0x0, 4, 1, 0x3B83126E),
    /* 0x854390 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9BC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8543B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC2B9C0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8543D0 call to 0x6D0450 */
    EXEDATA_ROW(Unhandled, 0x8543D0, 0x0, 0, 0, 0x0), // call to 0x6D0450
    /* 0x854410 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3804C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854430 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC38050, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC38054, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854470 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC38058, 0x0, 4, 1, 0x39D1B717),
    /* 0x854490 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3805C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8544B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC38060, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8544D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC39ED8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8544F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1E4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854510 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1E8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854530 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1EC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854550 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1F0, 0x0, 4, 1, 0x39D1B717),
    /* 0x854570 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1F4, 0x0, 4, 1, 0x3B83126E),
    /* 0x854590 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3A1F8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8545B0 _eh_vector_constructor_iterator(ctor 0x6FA6D0 dtor 0x6F9E00 64 x 0x24) */
    EXEDATA_ROW(VectorCtor, 0xC3A200, 0x6FA6D0, 36, 64, 0x0), // _eh_vector_constructor_iterator(ctor 0x6FA6D0 dtor 0x6F9E00 64 x 0x24)
    /* 0x8545E0 _eh_vector_constructor_iterator(ctor 0x6FA790 dtor 0x6F9E60 64 x 0x94) */
    EXEDATA_ROW(VectorCtor, 0xC3BB00, 0x6FA790, 148, 64, 0x0), // _eh_vector_constructor_iterator(ctor 0x6FA790 dtor 0x6F9E60 64 x 0x94)
    /* 0x854610 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E03C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854630 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E040, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854650 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E044, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854670 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E048, 0x0, 4, 1, 0x39D1B717),
    /* 0x854690 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E04C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8546B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3E050, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8546D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3EFB4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8546F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3F038, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854710 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3F044, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854730 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC3F0D8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854750 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402E8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854770 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402EC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854790 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402F0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8547B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402F4, 0x0, 4, 1, 0x39D1B717),
    /* 0x8547D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402F8, 0x0, 4, 1, 0x3B83126E),
    /* 0x8547F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC402FC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854810 1 row(s) */
    EXEDATA_ROW(StoreBytes, 0xC40300, 0x0, 4, 1, 0x42480000),
    /* 0x854820 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x854820, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x854840 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x854840, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x854860 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x854860, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x854880 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x854880, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x8548A0 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x8548A0, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x8548B0 1 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x8548B0, 0x0, 0, 0, 0x0), // 1 write(s) already zero in the native build
    /* 0x8548C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40338, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8548E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC4033C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40340, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40344, 0x0, 4, 1, 0x39D1B717),
    /* 0x854940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40348, 0x0, 4, 1, 0x3B83126E),
    /* 0x854960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC4034C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854980 mov ecx, 0xc40350 ; call 0x706770 */
    EXEDATA_ROW(ObjectCtor, 0xC40350, 0x706770, 0, 0, 0x0), // mov ecx, 0xc40350 ; call 0x706770
    /* 0x8549A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40418, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8549C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC4041C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8549E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40420, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854A00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40424, 0x0, 4, 1, 0x39D1B717),
    /* 0x854A20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC40428, 0x0, 4, 1, 0x3B83126E),
    /* 0x854A40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC4042C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854A60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC4B6B0, 0x0, 4, 1, 0x43610000),
    /* 0x854A80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A160, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854AA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A17C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854AC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A180, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854AE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A184, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854B00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A188, 0x0, 4, 1, 0x39D1B717),
    /* 0x854B20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A18C, 0x0, 4, 1, 0x3B83126E),
    /* 0x854B40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A190, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854B60 _eh_vector_constructor_iterator(ctor 0x70F8D0 dtor 0x70F8E0 64 x 0x1C) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x854B60, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x70F8D0 dtor 0x70F8E0 64 x 0x1C) (element ctor only clears)
    /* 0x854B90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6A8A8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854BB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA84, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854BD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA88, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854BF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA8C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854C10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA90, 0x0, 4, 1, 0x39D1B717),
    /* 0x854C30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA94, 0x0, 4, 1, 0x3B83126E),
    /* 0x854C50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6AA98, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854C70 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x854C70, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x854C80 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x854C80, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x854C90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E990, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854CB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E994, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854CD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E998, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854CF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E99C, 0x0, 4, 1, 0x39D1B717),
    /* 0x854D10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E9A0, 0x0, 4, 1, 0x3B83126E),
    /* 0x854D30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC6E9A4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854D50 _eh_vector_constructor_iterator(ctor 0x717110 dtor 0x717130 32 x 0x150) */
    EXEDATA_ROW(VectorCtor, 0xC6E9A8, 0x717110, 336, 32, 0x0), // _eh_vector_constructor_iterator(ctor 0x717110 dtor 0x717130 32 x 0x150)
    /* 0x854D80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71A58, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854DA0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x854DA0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x854DB0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x854DB0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x854DC0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 2 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x854DC0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 2 x 0x4) (element ctor only clears)
    /* 0x854DF0 _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 15 x 0x4) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x854DF0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x727230 dtor 0x7281E0 15 x 0x4) (element ctor only clears)
    /* 0x854E20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BDC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854E40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BE0, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854E60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BE4, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854E80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BE8, 0x0, 4, 1, 0x39D1B717),
    /* 0x854EA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BEC, 0x0, 4, 1, 0x3B83126E),
    /* 0x854EC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC71BF0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854EE0 _eh_vector_constructor_iterator(ctor 0x71A8B0 dtor 0x71A8C0 45 x 0x70) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x854EE0, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x71A8B0 dtor 0x71A8C0 45 x 0x70) (element ctor only clears)
    /* 0x854F10 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C34, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854F30 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C38, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x854F50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C3C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x854F70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C40, 0x0, 4, 1, 0x39D1B717),
    /* 0x854F90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C44, 0x0, 4, 1, 0x3B83126E),
    /* 0x854FB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C48, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x854FD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C4C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x854FF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC73C60, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855010 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC785F4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855030 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC785F8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855050 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC785FC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855070 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC78600, 0x0, 4, 1, 0x39D1B717),
    /* 0x855090 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC78604, 0x0, 4, 1, 0x3B83126E),
    /* 0x8550B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC78608, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8550D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799B0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8550F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799B4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855110 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799B8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855130 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799BC, 0x0, 4, 1, 0x39D1B717),
    /* 0x855150 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799C0, 0x0, 4, 1, 0x3B83126E),
    /* 0x855170 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC799C4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855190 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79A8C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8551B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79A90, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8551D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79A94, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8551F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79A98, 0x0, 4, 1, 0x39D1B717),
    /* 0x855210 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79A9C, 0x0, 4, 1, 0x3B83126E),
    /* 0x855230 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC79AA0, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855250 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C72C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855270 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C730, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855290 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C734, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8552B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C738, 0x0, 4, 1, 0x39D1B717),
    /* 0x8552D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C73C, 0x0, 4, 1, 0x3B83126E),
    /* 0x8552F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC7C740, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855310 _eh_vector_constructor_iterator(ctor 0x720F60 dtor 0x720F70 32 x 0xA0) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x855310, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x720F60 dtor 0x720F70 32 x 0xA0) (element ctor only clears)
    /* 0x855340 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80548, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855360 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80724, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855380 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80728, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8553A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8072C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8553C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80730, 0x0, 4, 1, 0x39D1B717),
    /* 0x8553E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80734, 0x0, 4, 1, 0x3B83126E),
    /* 0x855400 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC80738, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855420 _eh_vector_constructor_iterator(ctor 0x728B10 dtor 0x728B30 3 x 0x3CC) */
    EXEDATA_ROW(VectorCtor, 0xC80740, 0x728B10, 972, 3, 0x0), // _eh_vector_constructor_iterator(ctor 0x728B10 dtor 0x728B30 3 x 0x3CC)
    /* 0x855450 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81344, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855470 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81348, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855490 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8134C, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8554B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81350, 0x0, 4, 1, 0x39D1B717),
    /* 0x8554D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81354, 0x0, 4, 1, 0x3B83126E),
    /* 0x8554F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81358, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855510 mov ecx, 0xc81360 ; call 0x72a620 */
    EXEDATA_ROW(ObjectCtor, 0xC81360, 0x72A620, 0, 0, 0x0), // mov ecx, 0xc81360 ; call 0x72a620
    /* 0x855530 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC81454, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855550 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC815BC, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855570 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87AD0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855590 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87AD4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8555B0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87AD8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8555D0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87ADC, 0x0, 4, 1, 0x39D1B717),
    /* 0x8555F0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87AE0, 0x0, 4, 1, 0x3B83126E),
    /* 0x855610 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87AE4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855630 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87B0C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855650 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC87B38, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855670 mov ecx, 0xc87b40 ; jmp 0x72e860 */
    EXEDATA_ROW(ObjectCtor, 0xC87B40, 0x72E860, 0, 0, 0x0), // mov ecx, 0xc87b40 ; jmp 0x72e860
    /* 0x855680 mov ecx, 0xc87b88 ; jmp 0x72e860 */
    EXEDATA_ROW(ObjectCtor, 0xC87B88, 0x72E860, 0, 0, 0x0), // mov ecx, 0xc87b88 ; jmp 0x72e860
    /* 0x855690 mov ecx, 0xc87bd0 ; jmp 0x72e860 */
    EXEDATA_ROW(ObjectCtor, 0xC87BD0, 0x72E860, 0, 0, 0x0), // mov ecx, 0xc87bd0 ; jmp 0x72e860
    /* 0x8556A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88000, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8556C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8801C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8556E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88058, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855700 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8805C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855720 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88060, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855740 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88064, 0x0, 4, 1, 0x39D1B717),
    /* 0x855760 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88068, 0x0, 4, 1, 0x3B83126E),
    /* 0x855780 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8806C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8557A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8870C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x8557C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88710, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8557E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88714, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855800 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88718, 0x0, 4, 1, 0x39D1B717),
    /* 0x855820 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8871C, 0x0, 4, 1, 0x3B83126E),
    /* 0x855840 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88720, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855860 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88728, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855880 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8872C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x8558A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88730, 0x0, 4, 1, 0x41DE38E4),
    /* 0x8558C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88734, 0x0, 4, 1, 0x39D1B717),
    /* 0x8558E0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC88738, 0x0, 4, 1, 0x3B83126E),
    /* 0x855900 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8873C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855920 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888B8, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855940 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888BC, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855960 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888C0, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855980 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888C4, 0x0, 4, 1, 0x39D1B717),
    /* 0x8559A0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888C8, 0x0, 4, 1, 0x3B83126E),
    /* 0x8559C0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC888CC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x8559E0 mov ecx, 0xc888d0 ; call 0x5074b0 */
    EXEDATA_ROW(ObjectCtor, 0xC888D0, 0x5074B0, 0, 0, 0x0), // mov ecx, 0xc888d0 ; call 0x5074b0
    /* 0x855A00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89190, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855A20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89194, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855A40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89198, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855A60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8919C, 0x0, 4, 1, 0x39D1B717),
    /* 0x855A80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC891A0, 0x0, 4, 1, 0x3B83126E),
    /* 0x855AA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC891A4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855AC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89678, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855AE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8967C, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855B00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89680, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855B20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89684, 0x0, 4, 1, 0x39D1B717),
    /* 0x855B40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC89688, 0x0, 4, 1, 0x3B83126E),
    /* 0x855B60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8968C, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855B80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7C4, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855BA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7C8, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855BC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7CC, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855BE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7D0, 0x0, 4, 1, 0x39D1B717),
    /* 0x855C00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7D4, 0x0, 4, 1, 0x3B83126E),
    /* 0x855C20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A7D8, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855C40 mov ecx, 0xc8a7dc ; call 0x40fb60 */
    EXEDATA_ROW(ObjectCtor, 0xC8A7DC, 0x40FB60, 0, 0, 0x0), // mov ecx, 0xc8a7dc ; call 0x40fb60
    /* 0x855C60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A81C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855C80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A820, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855CA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A824, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855CC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A828, 0x0, 4, 1, 0x39D1B717),
    /* 0x855CE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A82C, 0x0, 4, 1, 0x3B83126E),
    /* 0x855D00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8A830, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855D20 _eh_vector_constructor_iterator(ctor 0x742A90 dtor 0x742AA0 2 x 0x2C) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x855D20, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x742A90 dtor 0x742AA0 2 x 0x2C) (element ctor only clears)
    /* 0x855D50 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAA0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855D70 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAA4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855D90 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAA8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855DB0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAAC, 0x0, 4, 1, 0x39D1B717),
    /* 0x855DD0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAB0, 0x0, 4, 1, 0x3B83126E),
    /* 0x855DF0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8AAB4, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855E10 _eh_vector_constructor_iterator(ctor 0x743C30 dtor 0x743C40 80 x 0x70) (element ctor only clears) */
    EXEDATA_ROW(Skip, 0x855E10, 0x0, 0, 0, 0x0), // _eh_vector_constructor_iterator(ctor 0x743C30 dtor 0x743C40 80 x 0x70) (element ctor only clears)
    /* 0x855E40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDD0, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855E60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDD4, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855E80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDD8, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855EA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDCC, 0x0, 4, 1, 0x39D1B717),
    /* 0x855EC0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDE0, 0x0, 4, 1, 0x3B83126E),
    /* 0x855EE0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC8CDDC, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855F00 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC9211C, 0x0, 4, 1, 0x39A3D70A),
    /* 0x855F20 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC92120, 0x0, 4, 1, 0x3CA3D70A),
    /* 0x855F40 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC92124, 0x0, 4, 1, 0x41DE38E4),
    /* 0x855F60 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC92128, 0x0, 4, 1, 0x39D1B717),
    /* 0x855F80 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC9212C, 0x0, 4, 1, 0x3B83126E),
    /* 0x855FA0 1 row(s) */
    EXEDATA_ROW(StoreFloat, 0xC92130, 0x0, 4, 1, 0x3BB60B6A),
    /* 0x855FC0 no effect (destructor registration and/or writes that are already zero) */
    EXEDATA_ROW(Skip, 0x855FC0, 0x0, 0, 0, 0x0), // no effect (destructor registration and/or writes that are already zero)
    /* 0x855FD0 8 write(s) already zero in the native build */
    EXEDATA_ROW(Skip, 0x855FD0, 0x0, 0, 0, 0x0), // 8 write(s) already zero in the native build
};

#undef EXEDATA_ROW

//! Little-endian payload bytes of a row, independent of the host's byte order.
std::array<std::uint8_t, 8> PayloadOf(const ExeDataEntry& e) {
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); i++) {
        bytes[i] = static_cast<std::uint8_t>(e.Value >> (i * 8));
    }
    return bytes;
}
} // namespace

std::span<const ExeDataEntry> GetExeDataEntries() {
    return kEntries;
}

void ApplyExeData() {
    std::size_t applied{}, bytesWritten{}, skipped{}, manual{};
    std::size_t unhandled{};    //!< Unhandled rows no manual handler claimed
    std::size_t constructions{};  //!< `ObjectCtor`/`VectorCtor` rows no manual handler claimed

    for (const auto& e : GetExeDataEntries()) {
        switch (e.Kind) {
        case ExeDataKind::StoreBytes:
        case ExeDataKind::StoreFloat: {
            const auto payload = PayloadOf(e);
            ExeGlobals::Write(e.Addr, payload.data(), e.Size);
            bytesWritten += e.Size;
            applied++;
            break;
        }
        case ExeDataKind::ZeroFill:
            ExeGlobals::ZeroFill(e.Addr, e.Size);
            bytesWritten += e.Size;
            applied++;
            break;
        case ExeDataKind::FillPattern:
            for (std::uint32_t i = 0; i < e.Count; i++) {
                const auto payload = PayloadOf(e);
                ExeGlobals::Write(e.Addr + i * e.Aux, payload.data(), e.Size);
            }
            bytesWritten += static_cast<std::size_t>(e.Count) * e.Size;
            applied++;
            break;
        case ExeDataKind::ObjectCtor:
        case ExeDataKind::VectorCtor:
        case ExeDataKind::Unhandled:
            if (ApplyManualExeDataEntry(e)) {
                manual++;
            } else if (e.Kind == ExeDataKind::Unhandled) {
                unhandled++;
                spdlog::warn("ExeData: initializer 0x{:X} left unhandled", e.Addr);
            } else {
                constructions++;
            }
            break;
        case ExeDataKind::Skip:
            skipped++;
            break;
        }
    }

    spdlog::info(
        "ExeData: applied {} rows ({} bytes), {} manual, {} skipped, {} constructors not applied, {} unhandled",
        applied, bytesWritten, manual, skipped, constructions, unhandled
    );
}
} // namespace notsa::native

#endif // NOTSA_NATIVE
