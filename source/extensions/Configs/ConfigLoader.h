#pragma once

namespace notsa {

//! Name of the INI file. It lives next to the game executable (or the game directory, natively).
inline constexpr auto DEFAULT_INI_FILENAME = "gta-reversed.ini";

/*!
* @brief Load `gta-reversed.ini` and apply every configuration section.
*
* The file is optional: if it doesn't exist all the configurations keep their defaults.
*/
void LoadConfigurations();

}; // namespace notsa
