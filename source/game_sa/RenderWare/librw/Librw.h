#pragma once

//! Umbrella include for librw.
//!
//! librw's headers have no include guards (they are only meant to be pulled in once, by `<rw.h>`),
//! so include this wrapper instead of `<rw.h>` directly: it makes repeated inclusion harmless.

#include <rw.h>
