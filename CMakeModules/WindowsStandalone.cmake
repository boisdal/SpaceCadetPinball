include(FetchContent)
# Release tags resolved to immutable commits; archive hashes verify every build.
FetchContent_Declare(pinball_sdl
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/fa24d868ac2f8fd558e4e914c9863411245db8fd
    URL_HASH SHA256=d42659685433a84f8b93f6f317a327c6599e190d910081615933abc96c1ebcb6)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_LIBC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pinball_sdl)
FetchContent_Declare(pinball_mixer
    URL https://codeload.github.com/libsdl-org/SDL_mixer/tar.gz/a37e09f85d321a13dfcf0d4432827ee09beeb623
    URL_HASH SHA256=d4a53753765b0b08fec9ecc66ca5b8a91efd6d015c6aea2c242d1517c52c7edf)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
# Historical resources use WAV and MIDI (MDS is converted by midi.cpp).
# No downloaded codec libraries or external soundfonts are required.
foreach(codec CMD FLAC GME MOD MP3 OPUS WAVPACK MIDI_FLUIDSYNTH MIDI_TIMIDITY)
    set(SDL2MIXER_${codec} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDL2MIXER_VORBIS OFF CACHE STRING "" FORCE)
set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI ON CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI_NATIVE ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pinball_mixer)
