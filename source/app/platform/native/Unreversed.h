#pragma once

#include <cstdint>

namespace notsa::native {
/*!
* @brief Report a call into the game executable that has no native implementation yet.
*
* Logs a critical message with the address (or vtable index) that was about to be called,
* flushes the logs and aborts. Never returns, so it can be used as the body of any function
* returning any type.
*
* @param addr The address (or vtable index) of the executable function.
* @param kind Either "function" or "vtable index".
*/
[[noreturn]] void UnreversedCall(uintptr_t addr, const char* kind = "function");
} // namespace notsa::native
