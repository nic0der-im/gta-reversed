/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "Font.h"

#include "eLanguage.h"

auto& FontRenderStateBuf = StaticRef<std::array<CFontChar, 9>>(0xC716B0);
auto& pEmptyChar = StaticRef<CFontChar*>(0xC716A8);

auto& gFontData = StaticRef<std::array<tFontData, 2>>(0xC718B0);

void CFont::InjectHooks() {
    RH_ScopedClass(CFont);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Initialise, 0x5BA690);
    RH_ScopedInstall(Shutdown, 0x7189B0);
    RH_ScopedInstall(PrintChar, 0x718A10);
    RH_ScopedInstall(ParseToken, 0x718F00);

    // styling functions
    RH_ScopedInstall(SetScale, 0x719380);
    RH_ScopedInstall(SetScaleForCurrentLanguage, 0x7193A0);
    RH_ScopedInstall(SetSlantRefPoint, 0x719400);
    RH_ScopedInstall(SetSlant, 0x719420);
    RH_ScopedInstall(SetColor, 0x719430);
    RH_ScopedInstall(SetFontStyle, 0x719490);
    RH_ScopedInstall(SetWrapx, 0x7194D0);
    RH_ScopedInstall(SetCentreSize, 0x7194E0);
    RH_ScopedInstall(SetRightJustifyWrap, 0x7194F0);
    RH_ScopedInstall(SetAlphaFade, 0x719500);
    RH_ScopedInstall(SetDropColor, 0x719510);
    RH_ScopedInstall(SetDropShadowPosition, 0x719570);
    RH_ScopedInstall(SetEdge, 0x719590);
    RH_ScopedInstall(SetProportional, 0x7195B0);
    RH_ScopedInstall(SetBackground, 0x7195C0);
    RH_ScopedInstall(SetBackgroundColor, 0x7195E0);
    RH_ScopedInstall(SetJustify, 0x719600);
    RH_ScopedInstall(SetOrientation, 0x719610);

    RH_ScopedInstall(InitPerFrame, 0x719800);
    RH_ScopedInstall(RenderFontBuffer, 0x719840);
    RH_ScopedInstall(GetStringWidth, 0x71A0E0);
    RH_ScopedInstall(DrawFonts, 0x71A210);
    RH_ScopedInstall(ProcessCurrentString, 0x71A220);
    RH_ScopedInstall(GetNumberLines, 0x71A5E0);
    RH_ScopedInstall(ProcessStringToDisplay, 0x71A600);
    RH_ScopedInstall(GetTextRect, 0x71A620);
    RH_ScopedInstall(PrintString, 0x71A700);
    RH_ScopedInstall(PrintStringFromBottom, 0x71A820);
    RH_ScopedInstall(GetCharacterSize, 0x719750);
    RH_ScopedInstall(LoadFontValues, 0x7187C0);
    RH_ScopedGlobalInstall(GetScriptLetterSize, 0x719670);
    RH_ScopedInstall(FindSubFontCharacter, 0x7192C0, { .StackArgumentsToPreserve = 2, .PreserveRegisters = true });
    RH_ScopedGlobalInstall(GetLetterIdPropValue, 0x718770);
}

// 0x7187C0
void CFont::LoadFontValues() {
    CFileMgr::SetDir("");
    auto* file = CFileMgr::OpenFile("DATA\\FONTS.DAT", "rb");

    char attrib[32];

    uint32 totalFonts = 0;
    uint32 fontId = 0;

    for (auto line = CFileLoader::LoadLine(file); line; line = CFileLoader::LoadLine(file)) {
        if (*line == '\0' || *line == '#')
            continue;

        if (sscanf_s(line, "%s", SCANF_S_STR(attrib)) == EOF)
            continue;

        if (!memcmp(attrib, "[TOTAL_FONTS]", 14)) {
            auto nextLine = CFileLoader::LoadLine(file);

            VERIFY(sscanf_s(nextLine, "%d", &totalFonts) == 1);
        }
        else if (!memcmp(attrib, "[FONT_ID]", 10)) {
            auto nextLine = CFileLoader::LoadLine(file);

            VERIFY(sscanf_s(nextLine, "%d", &fontId) == 1);
        }
        else if (!memcmp(attrib, "[REPLACEMENT_SPACE_CHAR]", 25)) {
            auto nextLine = CFileLoader::LoadLine(file);
            uint8 spaceValue;

            VERIFY(sscanf_s(nextLine, "%hhu", &spaceValue) == 1);
            gFontData[fontId].m_spaceValue = spaceValue;
        }
        else if (!memcmp(attrib, "[PROP]", 7)) {
            for (int32 i = 0; i < 26; i++) {
                auto nextLine = CFileLoader::LoadLine(file);
                int32 propValues[8]{};

                VERIFY(sscanf_s(nextLine, "%d  %d  %d  %d  %d  %d  %d  %d",
                    &propValues[0], &propValues[1], &propValues[2], &propValues[3],
                    &propValues[4], &propValues[5], &propValues[6], &propValues[7]
                ) == 8);

                for (auto j = 0u; j < std::size(propValues); j++) {
                    gFontData[fontId].m_propValues[i * 8 + j] = propValues[j];
                }
            }
        }
        else if (!memcmp(attrib, "[UNPROP]", 9)) {
            auto nextLine = CFileLoader::LoadLine(file);
            uint32 unpropValue;

            VERIFY(sscanf_s(nextLine, "%d", &unpropValue) == 1);
            gFontData[fontId].m_unpropValue = unpropValue;
        }
    }

    CFileMgr::CloseFile(file);
}

// 0x5BA690
void CFont::Initialise() {
    int32 fontsTxd = CTxdStore::AddTxdSlot("fonts");
    CTxdStore::LoadTxd(fontsTxd, "MODELS\\FONTS.TXD");
    CTxdStore::AddRef(fontsTxd);
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(fontsTxd);
    Sprite[0].SetTexture("font2", "font2m");
    Sprite[1].SetTexture("font1", "font1m");

    LoadFontValues();

    SetScale(1.0f, 1.0f);
    SetSlantRefPoint(SCREEN_WIDTH, 0.0f);
    SetSlant(0.0f);

    SetColor(CRGBA(255, 255, 255, 0));
    SetOrientation(eFontAlignment::ALIGN_LEFT);
    SetJustify(false);

    SetWrapx(SCREEN_WIDTH);

    SetCentreSize(SCREEN_WIDTH);
    SetBackground(false, false);

    SetBackgroundColor(CRGBA(128, 128, 128, 128));
    SetProportional(true);
    SetFontStyle(eFontStyle::FONT_GOTHIC);
    SetRightJustifyWrap(0.0f);
    SetAlphaFade(255.0f);
    SetDropShadowPosition(0);
    CTxdStore::PopCurrentTxd();

    int32 ps2btnsTxd = CTxdStore::AddTxdSlot("ps2btns");
    CTxdStore::LoadTxd(ps2btnsTxd, "MODELS\\PCBTNS.TXD");
    CTxdStore::AddRef(ps2btnsTxd);
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(ps2btnsTxd);

    ButtonSprite[1].SetTexture("up", "upA");
    ButtonSprite[2].SetTexture("down", "downA");
    ButtonSprite[3].SetTexture("left", "leftA");
    ButtonSprite[4].SetTexture("right", "rightA");

    CTxdStore::PopCurrentTxd();
}

// 0x7189B0
void CFont::Shutdown() {
    std::ranges::for_each(Sprite, [](CSprite2d& sprite) { sprite.Delete(); });
    CTxdStore::SafeRemoveTxdSlot("fonts"); // FIX_BUGS: Added check for is slot exists
    std::ranges::for_each(ButtonSprite, [](CSprite2d& sprite) { sprite.Delete(); });
    CTxdStore::SafeRemoveTxdSlot("ps2btns"); // FIX_BUGS: Added check for is slot exists
}

// this adds a single character into rendering buffer
// 0x718A10
void CFont::PrintChar(float x, float y, uint8 character) {
    // Out of screen
    if (y < 0.0f || y > SCREEN_HEIGHT || x < 0.0f || x > SCREEN_WIDTH)
        return;

    if (PS2Symbol != EXSYMBOL_NONE) {
        // Extra symbol to be drawn (e.g. PS2 buttons)
        ButtonSprite[PS2Symbol].Draw(
            CRect{
                x,
                RenderState.m_fHeight * 2.0f  + y,
                RenderState.m_fHeight * 17.0f + x,
                RenderState.m_fHeight * 19.0f + y
            },
            CRGBA{ 255, 255, 255, RenderState.m_color.a }
        );
        return;
    }

    bool skipDraw = false;
    if (character == 0 || character == '?') {
        // Space or a missing glyph - Don't draw anything
        character = 0;
        skipDraw  = true;
    }

    const float propValue = GetLetterIdPropValue(character) * 0.03125f; // 1/32

    if (RenderState.m_nFontStyle == 1 && character == 0xD0)
        character = 0;

    const float u0 = static_cast<float>(character & 0xF) * 0.0625f; // 1/16 (32px cell on a 512px wide atlas)

    if (RenderState.m_wFontTexture == 0 || RenderState.m_wFontTexture == 1) {
        // The two main fonts have 32x40 cells (40/512 = 0.078125)
        const float v0 = static_cast<float>(character >> 4) * 0.078125f;

        if (skipDraw)
            return;

        const float u3 = u0 + 0.0625f - 0.001f;

        if (RenderState.m_fSlant != 0.0f) {
            // Slanted text (The shear itself is applied by `RenderFontBuffer`)
            CSprite2d::AddToBuffer(
                CRect{
                    x,
                    y + 0.015f,
                    RenderState.m_fWidth * 32.0f + x,
                    RenderState.m_fHeight * 40.0f * 0.5f + y + 0.015f
                },
                RenderState.m_color,
                u0, v0 + 0.00055f,
                u3, v0 + 0.0121f,
                u0, v0 + 0.078125f - 0.009f,
                u3, v0 + 0.078125f - 0.0021f + 0.01f
            );
        } else if (character < 0xC0) {
            // Normal glyph (20 * scaleY tall)
            CSprite2d::AddToBuffer(
                CRect{
                    x,
                    y,
                    RenderState.m_fWidth * 32.0f + x,
                    RenderState.m_fHeight * 40.0f * 0.5f + y
                },
                RenderState.m_color,
                u0, v0 + 0.0021f,
                u3, v0 + 0.0021f,
                u0, v0 + 0.078125f - 0.0021f,
                u3, v0 + 0.078125f - 0.0021f
            );
        } else {
            // Half height glyph (16 * scaleY tall)
            CSprite2d::AddToBuffer(
                CRect{
                    x,
                    y,
                    RenderState.m_fWidth * 32.0f + x,
                    RenderState.m_fHeight * 32.0f * 0.5f + y
                },
                RenderState.m_color,
                u0, v0 + 0.0021f,
                u3, v0 + 0.0021f,
                u0, v0 + 0.078125f - 0.016f,
                u3, v0 + 0.078125f - 0.015f
            );
        }

        return;
    }

    // Fonts 2..N have square 32x32 cells
    const float v0 = static_cast<float>(character >> 4) * 0.0625f;

    if (skipDraw)
        return;

    const float u3 = u0 + propValue * 0.0625f;

    CSprite2d::AddToBuffer(
        CRect{
            x,
            y,
            RenderState.m_fWidth * 32.0f * propValue + x,
            RenderState.m_fHeight * 32.0f * 0.5f + y
        },
        RenderState.m_color,
        u0,           v0,
        u3,           v0,
        u0,           v0 + 0.0625f,
        u3 - 0.0001f, v0 + 0.0625f - 0.0001f
    );
}

// Tags processing
// 0x718F00
char* CFont::ParseToken(char* text, CRGBA& color, bool isBlip, char* tag) {
    // info about tokens: https://gtamods.com/wiki/GXT#Tokens

    char* next = ++text;

    auto ApplyStyle = [&](eHudColours hudColor) {
        if (!isBlip)
            color = HudColour.GetRGBA(hudColor, color.a);

        if (tag)
            *tag = *next;
    };

    switch (*next) {
    case '<':
        PS2Symbol = EXSYMBOL_DPAD_LEFT;
        break;
    case '>':
        PS2Symbol = EXSYMBOL_DPAD_RIGHT;
        break;
    case 'A':
    case 'a':
        PS2Symbol = EXSYMBOL_L3;
        break;
    case 'B':
    case 'b':
        ApplyStyle(HUD_COLOUR_DARK_BLUE);
        break;
    case 'C':
    case 'c':
        PS2Symbol = EXSYMBOL_R3;
        break;
    case 'D':
    case 'd':
        PS2Symbol = EXSYMBOL_DPAD_DOWN;
        break;
    case 'G':
    case 'g':
        ApplyStyle(HUD_COLOUR_GREEN);
        break;
    case 'H':
    case 'h':
        if (!isBlip) {
            color = {
                (uint8)std::min((float)color.r * 1.5f, 255.0f),
                (uint8)std::min((float)color.g * 1.5f, 255.0f),
                (uint8)std::min((float)color.b * 1.5f, 255.0f),
                color.a
            };
        }

        if (tag)
            *tag = *next;
        break;
    case 'J':
    case 'j':
        PS2Symbol = EXSYMBOL_R1;
        break;
    case 'K':
    case 'k':
        PS2Symbol = EXSYMBOL_KEY;
        break;
    case 'M':
    case 'm':
        PS2Symbol = EXSYMBOL_L2;
        break;
    case 'N':
    case 'n':
        m_bNewLine = true;
        break;
    case 'O':
    case 'o':
        PS2Symbol = EXSYMBOL_CIRCLE;
        break;
    case 'P':
    case 'p':
        ApplyStyle(HUD_COLOUR_PURPLE);
        break;
    case 'Q':
    case 'q':
        PS2Symbol = EXSYMBOL_SQUARE;
        break;
    case 'R':
    case 'r':
        ApplyStyle(HUD_COLOUR_RED);
        break;
    case 'S':
    case 's':
        ApplyStyle(HUD_COLOUR_LIGHT_GRAY);
        break;
    case 'T':
    case 't':
        PS2Symbol = EXSYMBOL_TRIANGLE;
        break;
    case 'U':
    case 'u':
        PS2Symbol = EXSYMBOL_DPAD_UP;
        break;
    case 'V':
    case 'v':
        PS2Symbol = EXSYMBOL_R2;
        break;
    case 'W':
    case 'w':
        ApplyStyle(HUD_COLOUR_LIGHT_GRAY);
        break;
    case 'X':
    case 'x':
        PS2Symbol = EXSYMBOL_CROSS;
        break;
    case 'Y':
    case 'y':
        ApplyStyle(HUD_COLOUR_CREAM);
        break;
    case 'l':
        ApplyStyle(HUD_COLOUR_BLACK);
        break;
    default:
        break;
    }

    if (*next != '~') {
        // skip text to the next '~' character.
        for(; *next && *next != '~'; next++);
    }

    if (*next)
        return next + 1;

    return next + 2;
}

// Text scaling
// 0x719380
void CFont::SetScale(float w, float h) {
    m_Scale.Set(w, h);
}

// Text scaling depends on current language
// 0x7193A0
void CFont::SetScaleForCurrentLanguage(float w, float h) {
    switch (FrontEndMenuManager.m_nPrefsLanguage) {
    case eLanguage::FRENCH:
    case eLanguage::GERMAN:
    case eLanguage::ITALIAN:
    case eLanguage::SPANISH:
        m_Scale.Set(w * 0.8f, h);
        break;
    default:
        m_Scale.Set(w, h);
    }
}

// Set text rotation point
// 0x719400
void CFont::SetSlantRefPoint(float x, float y) {
    m_fSlantRefPoint.Set(x, y);
}

// Set text rotation angle
// 0x719420
void CFont::SetSlant(float value) {
    m_fSlant = value;
}

// Set text color
// 0x719430
void CFont::SetColor(CRGBA color) {
    m_Color = color;

    if (m_fFontAlpha < 255.0f) {
        m_Color.a = (uint8)(float(color.a) * m_fFontAlpha / 255.0f);
    }
}

// Set text style
// 0x719490
void CFont::SetFontStyle(eFontStyle style) {
    switch (style) {
    case eFontStyle::FONT_PRICEDOWN:
        m_FontTextureId = 1;
        m_FontStyle = 1;
        break;
    case eFontStyle::FONT_MENU:
        m_FontTextureId = 0;
        m_FontStyle = 2;
        break;
    default:
        m_FontTextureId = static_cast<uint8>(style);
        m_FontStyle = 0;
    }
}

// Set line width at right
// 0x7194D0
void CFont::SetWrapx(float value) {
    m_fWrapx = value;
}

// Set line width at center
// 0x7194E0
void CFont::SetCentreSize(float value) {
    m_fFontCentreSize = value;
}

// 0x7194F0
void CFont::SetRightJustifyWrap(float value) {
    m_fRightJustifyWrap = value;
}

// Like a 'global' font alpha, multiplied with each text alpha (from SetColor)
// 0x719500
void CFont::SetAlphaFade(float alpha) {
    m_fFontAlpha = alpha;
}

// Drop color is used for text shadow and text outline
// 0x719510
void CFont::SetDropColor(CRGBA color) {
    m_FontDropColor = color;

    if (m_fFontAlpha < 255.0f) {
        m_FontDropColor.a = (uint8)(float(m_Color.a) * m_fFontAlpha);
    }
}

// Set shadow size
// 0x719570
void CFont::SetDropShadowPosition(int16 value) {
    m_nFontOutlineSize = 0;
    m_nFontOutlineOrShadow = 0;
    m_nFontShadow = (uint8)value;
}

// Set outline size
// 0x719590
void CFont::SetEdge(int8 value) {
    m_nFontShadow = 0;
    m_nFontOutlineSize = value;
    m_nFontOutlineOrShadow = value;
}

// Toggles character proportions in text
// 0x7195B0
void CFont::SetProportional(bool on) {
    m_bFontPropOn = on;
}

// Setups text background
// 0x7195C0
void CFont::SetBackground(bool enable, bool includeWrap) {
    m_bFontBackground = enable;
    m_bEnlargeBackgroundBox = includeWrap;
}

// Sets background color
// 0x7195E0
void CFont::SetBackgroundColor(CRGBA color) {
    m_FontBackgroundColor = color;
}

// 0x719600
void CFont::SetJustify(bool on) {
    m_bFontJustify = on;
}

// 0x719610
void CFont::SetOrientation(eFontAlignment alignment) {
    m_bFontCentreAlign = alignment == eFontAlignment::ALIGN_CENTER;
    m_bFontRightAlign = alignment == eFontAlignment::ALIGN_RIGHT;
}

// Need to call this each frame
// 0x719800
void CFont::InitPerFrame() {
    ZoneScoped;

    m_nFontOutline = 0;
    m_nFontOutlineOrShadow = 0;
    m_nFontShadow = 0;
    m_bNewLine = false;
    PS2Symbol = EXSYMBOL_NONE;
    RenderState.m_wFontTexture = 0; // todo: -1
    pEmptyChar = &FontRenderStateBuf[0]; // FontRenderStatePointer.pRenderState

    CSprite::InitSpriteBuffer();
}

// Draw text we have in buffer
// 0x719840
void CFont::RenderFontBuffer() {
    if (pEmptyChar == &FontRenderStateBuf[0])
        return;

    Sprite[RenderState.m_wFontTexture].SetRenderState();
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));

    RenderState.Set(FontRenderStateBuf[0]);

    CRGBA color = RenderState.m_color;
    float x     = RenderState.m_vPosn.x;
    float y     = RenderState.m_vPosn.y;

    // The first record holds the base state, the queued strings (and their state records) come after it
    auto* text = reinterpret_cast<char*>(&FontRenderStateBuf[1]);
    auto* end  = reinterpret_cast<char*>(pEmptyChar);

    while (text < end) {
        if (*text == '\0') {
            // End of the current string - Skip to the next (4 byte aligned) state record
            do {
                ++text;
            } while (reinterpret_cast<uintptr_t>(text) & 3);

            if (text >= end)
                break;

            RenderState.Set(*reinterpret_cast<const CFontChar*>(text));

            color = RenderState.m_color;
            x     = RenderState.m_vPosn.x;
            y     = RenderState.m_vPosn.y;

            text += sizeof(CFontChar);
        }

        PS2Symbol = EXSYMBOL_NONE;

        while (*text == '~' && PS2Symbol == EXSYMBOL_NONE) {
            text = ParseToken(text, color, RenderState.m_bContainImages, nullptr);
            if (!RenderState.m_bContainImages)
                RenderState.m_color = color;
        }

        uint8 letter = static_cast<uint8>(*text - 0x20);

        if (RenderState.m_nFontStyle == 0) {
            if (letter == 0x91) {
                letter = 0x40;
            } else if (letter > 0x9B) {
                letter = 0;
            }
        } else {
            letter = FindSubFontCharacter(letter, RenderState.m_nFontStyle);
        }

        if (RenderState.m_fSlant != 0.0f) { // Shear the line
            y = (RenderState.m_vSlanRefPoint.x - x) * RenderState.m_fSlant + RenderState.m_vSlanRefPoint.y;
        }

        if (PS2Symbol == EXSYMBOL_NONE || !RenderState.m_bContainImages)
            PrintChar(x, y, letter);

        if (PS2Symbol == EXSYMBOL_NONE) {
            const uint8 idx = letter == '?' ? 0 : letter;
            const uint8 adv = RenderState.m_bPropOn
                ? gFontData[RenderState.m_wFontTexture].m_propValues[idx]
                : gFontData[RenderState.m_wFontTexture].m_unpropValue;

            x += (static_cast<float>(static_cast<int8>(RenderState.m_nOutline)) + static_cast<float>(adv)) * RenderState.m_fWidth;
        } else {
            // Advance past the symbol
            x += RenderState.m_fHeight * 17.0f + static_cast<float>(static_cast<int8>(RenderState.m_nOutline));
        }

        if (letter == 0) // Space
            x += RenderState.m_fWrap;

        if (*text == '\0') {
            if (PS2Symbol != EXSYMBOL_NONE) {
                PS2Symbol = EXSYMBOL_NONE;
                Sprite[RenderState.m_wFontTexture].SetRenderState();
            }
        } else if (PS2Symbol == EXSYMBOL_NONE) {
            ++text;
        } else {
            PS2Symbol = EXSYMBOL_NONE;
            Sprite[RenderState.m_wFontTexture].SetRenderState();
        }
    }

    CSprite::FlushSpriteBuffer();
    CSprite2d::RenderVertexBuffer();
    pEmptyChar = &FontRenderStateBuf[0];
}

// 0x71A0E0
float CFont::GetStringWidth(const GxtChar* string, bool full, bool scriptText) {
    size_t len = CMessages::GetStringLength(string);
    GxtChar data[400] = { 0 };

    strncpy_s((char*)data, sizeof(data), AsciiFromGxtChar(string), len);
    CMessages::InsertPlayerControlKeysInString(data);

    float width = 0.0f;
    bool lastWasTag = false, lastWasLetter = false;
    auto* pStr = data;

    while (true) {
        if (*pStr == ' ' && !full)
            break;
        if (*pStr == '\0')
            break;

        if (*pStr == '~') {
            if (!full && (lastWasTag || lastWasLetter))
                return width;

            auto* next = pStr + 1;

            if (*next != '~') {
                for (; *next && *next != '~'; next++);
            }

            pStr = next + 1;

            if (lastWasLetter || *pStr == '~')
                lastWasTag = true;
        }
        else {
            if (!full && *pStr == ' ' && lastWasTag)
                return width;

            char upper = *pStr - 0x20;

            pStr++;
            if (scriptText) {
                width += GetScriptLetterSize(upper);
            }
            else {
                width += GetCharacterSize(upper);
            }

            lastWasLetter = true;
        }
    }

    return width;
}

// same as RenderFontBuffer() (0x71A210)
void CFont::DrawFonts() {
    ZoneScoped;

    RenderFontBuffer();
}

// 0x719B40
// Queues a `[start, end)` range of `text` to be laid out at (x, y) - `RenderFontBuffer` draws it later
static void QueueString(float x, float y, GxtChar* start, GxtChar* end, float spacing) {
#ifdef NOTSA_NATIVE
    // TODO(re): 0x719B40 queues glyphs into `FontRenderStateBuf`; until it is reversed, text is not drawn.
    (void)(x), (void)(y), (void)(start), (void)(end), (void)(spacing);
#else
    plugin::Call<0x719B40, float, float, GxtChar*, GxtChar*, float>(x, y, start, end, spacing);
#endif
}

// 0x71A220
int16 CFont::ProcessCurrentString(bool print, float x, float y, const GxtChar* text) {
    int16 spaceCount = 0; // Spaces in the current line
    int16 lineCount  = 0; // Lines the string takes up

    const CRGBA savedColor = m_Color;

    bool  firstOnLine      = true;  // Nothing was printed on the current line yet
    char  tokenChar        = '\0';  // Single character token reported by `ParseToken`
    float widthAtLastSpace = 0.0f;  // Width of the line up to the last space
    CRGBA tokenColor       = m_Color; // FIX_BUGS: Uninitialized in the original, yet `ParseToken` reads it (`~h~`)
    float lineWidth        = (m_bFontCentreAlign || m_bFontRightAlign) ? 0.0f : x; // Width of the text on the current line
    float lineY            = y;
    float drawX            = 0.0f;

    GxtChar lineBuf[256]; // Used to re-queue a line starting with a token

    if (*text != '\0') {
        auto* cursor    = const_cast<GxtChar*>(text);
        auto* wordStart = cursor;

        while (*cursor != '\0') {
            PS2Symbol = EXSYMBOL_NONE;

            const float wordWidth = GetStringWidth(cursor, false, false);

            if (*cursor == '~')
                cursor = reinterpret_cast<GxtChar*>(ParseToken(reinterpret_cast<char*>(cursor), tokenColor, true, &tokenChar));

            const float limit = m_bFontCentreAlign
                ? m_fFontCentreSize
                : (m_bFontRightAlign ? x - m_fRightJustifyWrap : m_fWrapx);

            if ((lineWidth + wordWidth > limit && !firstOnLine) || m_bNewLine) {
                // Current word doesn't fit - Start a new line
                float spacing = 0.0f;

                if (PS2Symbol != EXSYMBOL_NONE)
                    cursor -= 3; // Step back to the start of the `~x~` token

                auto* wordEnd = m_bNewLine ? cursor - 3 : cursor;

                if (m_bFontJustify && !m_bFontCentreAlign) {
                    // FIX_BUGS: Guard against a division by zero (Would result in an infinite spacing)
                    if (spaceCount != 0)
                        spacing = (m_fWrapx - widthAtLastSpace) / static_cast<float>(spaceCount);
                }

                if (m_bFontCentreAlign) {
                    drawX = x - lineWidth * 0.5f;
                } else if (m_bFontRightAlign) {
                    drawX = x - (lineWidth - GetCharacterSize(0));
                } else {
                    drawX = x;
                }

                ++lineCount;

                if (print)
                    QueueString(drawX, lineY, wordStart, wordEnd, spacing);

                if (tokenChar != '\0') {
                    // Rebuild the line, so it starts with the token (Which we've already parsed)
                    const auto prefixLen = std::snprintf(
                        reinterpret_cast<char*>(lineBuf),
                        std::size(lineBuf),
                        "~%c~",
                        static_cast<char>(tokenChar)
                    );

                    if (m_bNewLine)
                        wordEnd += 3; // Skip the newline token

                    // FIX_BUGS: Use a bounded copy (Originally the 256 byte `lineBuf` could be overflown)
                    if (prefixLen > 0 && static_cast<size_t>(prefixLen) < std::size(lineBuf))
                        std::strncat(reinterpret_cast<char*>(lineBuf), reinterpret_cast<const char*>(wordEnd), std::size(lineBuf) - 1 - static_cast<size_t>(prefixLen));

                    cursor    = lineBuf;
                    tokenChar = '\0';
                }

                lineY            += m_Scale.y * 32.0f * 0.5f + m_Scale.y + m_Scale.y; // 18 * scaleY
                m_bNewLine        = false;
                lineWidth         = (m_bFontCentreAlign || m_bFontRightAlign) ? 0.0f : x;
                spaceCount        = 0;
                widthAtLastSpace  = 0.0f;
                firstOnLine       = true;
                wordStart         = cursor;
            } else {
                lineWidth += wordWidth;

                while (*cursor != ' ' && *cursor != '\0' && *cursor != '~')
                    ++cursor;

                if (*cursor == '\0') {
                    // The last word of the string - Queue it right away
                    if (m_bFontCentreAlign) {
                        drawX = x - lineWidth * 0.5f;
                    } else if (m_bFontRightAlign) {
                        drawX = x - lineWidth;
                    } else {
                        drawX = x;
                    }

                    ++lineCount;

                    if (print)
                        QueueString(drawX, lineY, wordStart, cursor, 0.0f);
                } else {
                    if (!firstOnLine)
                        ++spaceCount;

                    if (*cursor != '~') { // Don't step over tokens
                        lineWidth += GetCharacterSize(0); // Account for the space
                        ++cursor;
                    }

                    widthAtLastSpace = lineWidth;
                    firstOnLine      = false;
                }
            }

            PS2Symbol = EXSYMBOL_NONE;
        }
    }

    if (print)
        SetColor(savedColor);

    return lineCount;
}

// 0x71A5E0
int16 CFont::GetNumberLines(float x, float y, const GxtChar* text) {
    return ProcessCurrentString(false, x, y, text);
}

// 0x71A600
int16 CFont::ProcessStringToDisplay(float x, float y, const GxtChar* text) {
    return ProcessCurrentString(true, x, y, text);
}

// 0x71A620
void CFont::GetTextRect(CRect* rect, float x, float y, const GxtChar* text) {
    if (m_bFontCentreAlign) {
        rect->left = x - (m_fFontCentreSize / 2.0f + 4.0f);
        rect->right = m_fFontCentreSize / 2.0f + x + 4.0f;
    }
    else if (m_bFontRightAlign) {
        rect->left = m_fRightJustifyWrap - 4.0f;
        rect->right = x + 4.0f;
    }
    else {
        rect->left = x - 4.0f;
        rect->right = m_fWrapx + 4.0f;
    }

    rect->top = y - 4.0f;
    rect->bottom = y + 4.0f + GetHeight() * (float)GetNumberLines(x, y, text);
}

// 0x71A700
void CFont::PrintString(float x, float y, const GxtChar* text) {
    if (*text == '\0' || *text == '*')
        return;

    if (m_bFontBackground) {
        CRect rt;

        RenderState.m_color = m_Color;
        GetTextRect(&rt, x, y, text);

        if (m_bEnlargeBackgroundBox) {
            rt.left -= 1.0f;
            rt.right += 1.0f;
            rt.bottom += 1.0f;
            rt.top -= 1.0f;

            FrontEndMenuManager.DrawWindow(rt, nullptr, 0, m_FontBackgroundColor, false, true);
        } else {
            CSprite2d::DrawRect(rt, m_FontBackgroundColor);
        }
        m_bFontBackground = false;
    }

    ProcessStringToDisplay(x, y, text);
}

// 0x71A820
void CFont::PrintStringFromBottom(float x, float y, const GxtChar* text) {
    float drawY = y - GetHeight() * (float)GetNumberLines(x, y, text);

    if (m_fSlant != 0.0f)
        drawY -= (m_fSlantRefPoint.x - x) * m_fSlant + m_fSlantRefPoint.y;

    PrintString(x, drawY, text);
}

// 0x719750
float CFont::GetCharacterSize(uint8 letterId) {
    uint8 propValueIdx = letterId;

    if (letterId == '?') {
        letterId = 0;
        propValueIdx = 0;
    }

    if (m_FontStyle)
        propValueIdx = FindSubFontCharacter(letterId, m_FontStyle);
    else if (propValueIdx == 145)
        propValueIdx = '@';
    else if (propValueIdx > 155)
        propValueIdx = 0;

    if (m_bFontPropOn) {
        return ((float)gFontData[m_FontTextureId].m_propValues[propValueIdx] + (float)m_nFontOutlineSize) * m_Scale.x;
    } else {
        return ((float)gFontData[m_FontTextureId].m_unpropValue + (float)m_nFontOutlineSize) * m_Scale.x;
    }
}

// Android
float CFont::GetHeight(bool a1) {
    assert(a1 == false && "NOT IMPLEMENTED");
    const float y = a1 ? 0.0f : m_Scale.y;
    return y * 32.0f / 2.0f + y + y;
}

// 0x719670, original name unknown
//! Advance width of `letterId` for the script text line currently being built: the current
//! `CTheScripts::IntroTextLines` entry supplies the scale, proportional flag, edge and style.
float GetScriptLetterSize(uint8 letterId) {
    const auto& text = CTheScripts::IntroTextLines[CTheScripts::NumberOfIntroTextLinesThisFrame];

    const uint8 ch = letterId == '?' ? 0 : letterId;

    uint8 row, col;
    if (text.FontStyle == FONT_MENU) {
        row = CFont::FindSubFontCharacter(ch, 2);
        col = 0;
    } else if (text.FontStyle == FONT_PRICEDOWN) {
        row = CFont::FindSubFontCharacter(ch, 1);
        col = 1;
    } else {
        col = static_cast<uint8>(+text.FontStyle);
        row = ch == 0x91 ? 0x40 : (ch > 0x9B ? 0 : ch);
    }

    // The executable indexes the flat font data block: the proportional table when the line is
    // proportional, otherwise the fixed width. `row` may reach `tFontData::m_spaceValue` (208), so
    // the block is addressed as bytes instead of `m_propValues[row]`.
    const uint8* const widths = reinterpret_cast<const uint8*>(&gFontData[col]);
    const uint8 width = text.IsProportional
        ? widths[row]
        : widths[offsetof(tFontData, m_unpropValue)];

    return (static_cast<float>(width) + static_cast<float>(text.TextEdge)) * text.Scale.x;
}

// 0x7192C0
uint8 CFont::FindSubFontCharacter(uint8 letterId, uint8 fontStyle) {
    if (fontStyle == 1) { // eFontStyle::FONT_PRICEDOWN
        switch (letterId) {
        case 1:  return 208;
        case 4:  return 93;
        case 7:  return 206;
        case 8:
        case 9:  return letterId + 86;
        case 14: return 207;
        case 26: return 154;
        }
    }

    if (letterId == 6)                      return 10;
    if (letterId >=  16 && letterId <=  25) return letterId - 128;
    if (letterId == 31)                     return 91;
    if (letterId >=  33 && letterId <=  58) return letterId + 122;
    if (letterId == 62)                     return 32;
    if (letterId >=  65 && letterId <=  90) return letterId + 90;
    if (letterId >=  96 && letterId <= 118) return letterId + 85;
    if (letterId >= 119 && letterId <= 140) return letterId + 62;
    if (letterId >= 141 && letterId <= 142) return 204;
    if (letterId == 143)                    return 205;

    return letterId;
}

// 0x718770
float GetLetterIdPropValue(uint8 letterId) {
    uint8 id = letterId;

    if (letterId == '?')
        id = 0;

    if (CFont::RenderState.m_bPropOn)
        return gFontData[CFont::RenderState.m_wFontTexture].m_propValues[id];
    else
        return gFontData[CFont::RenderState.m_wFontTexture].m_unpropValue;
}
