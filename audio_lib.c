/*
 * Builds the bundled audio libraries (see third_party/README.md). Added in
 * 2026 by dleecurt/craft; not part of the CRAFT 3.5 Standard Version.
 */
#define STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis/stb_vorbis.c"

#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include "third_party/miniaudio/miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis/stb_vorbis.c"
