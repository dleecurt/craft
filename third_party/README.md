# Third-party code

Bundled so that Craft needs no extra packages for sound. Both are
compiled into `libcraftaudio.a` by `audio_lib.c`, with warnings off
because they are used unchanged.

| Library | Version | Source | Licence |
|---|---|---|---|
| miniaudio | 0.11.26 (commit 877ea4a) | https://github.com/mackron/miniaudio | Public domain (Unlicense) or MIT No Attribution, at your choice: `miniaudio/LICENSE` |
| stb_vorbis | 1.23 (commit 78770d8) | https://github.com/nothings/stb | MIT or public domain (Unlicense), at your choice: `stb_vorbis/LICENSE` |

miniaudio plays through PipeWire, PulseAudio or ALSA, which it loads at
run time, and decodes WAV, FLAC and MP3; stb_vorbis adds Ogg Vorbis.
