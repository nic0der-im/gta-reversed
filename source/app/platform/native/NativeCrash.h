#pragma once

namespace notsa::native {
/*!
* @brief Install handlers for the fatal signals of the process.
*
* Every handled signal prints its number and a backtrace to stderr, restores the default
* disposition of the signal and re-raises it, so the process still dies from the original signal
* (and the debugger/shell sees it as a crash rather than a plain exit).
*
* Only valid on POSIX systems (it relies on `backtrace`), so it's a no-op elsewhere.
*/
void InstallCrashHandler();
} // namespace notsa::native
