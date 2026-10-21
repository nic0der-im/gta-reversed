#pragma once

//! Native-only debugging aid: a screenshot of the presented frame.
//!
//! Set `GTASA_CAPTURE=<path.png>` to dump the GL back buffer to that file after
//! `GTASA_CAPTURE_FRAME` presented frames (default 90) and then quit the game. Used to verify
//! what the native backend actually put on screen without a human looking at the window.

#ifdef NOTSA_NATIVE

namespace notsa::native {

//! Call right before the back buffer is swapped. Does nothing unless `GTASA_CAPTURE` is set.
void CaptureOnPresent();

} // namespace notsa::native

#endif // NOTSA_NATIVE
