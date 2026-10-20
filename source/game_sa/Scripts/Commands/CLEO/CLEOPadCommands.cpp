#include <StdInc.h>

#include "CLEOCommands.hpp"
#include "CommandParser/Parser.hpp"

/*!
* Various player pad commands
*/

bool IsKeyPressed(int32 virtualKey) {
#ifdef NOTSA_NATIVE
    //! Natively the keyboard state comes from SDL (through `CPad`); only keys in its
    //! `standardKeys` range are tracked.
    return virtualKey >= 0 && virtualKey < (int32)std::size(CPad::GetPad()->NewKeyState.standardKeys)
        && CPad::GetPad()->NewKeyState.standardKeys[virtualKey];
#else
    return GetKeyState(virtualKey) & 0x8000; // NOTE: TODO
#endif
}

void notsa::script::commands::cleo::pad::RegisterHandlers() {
    //REGISTER_COMMAND_HANDLER(COMMAND_IS_KEY_PRESSED, IsKeyPressed);
}
