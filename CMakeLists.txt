cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if("${MODEL}" MATCHES "gcw0")
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/mips32-linux-gcc.cmake")
  set(PROFILE_GEN_DIR "/media/data/profile/snes9x")
else()
  set(PROFILE_GEN_DIR "./profile/snes9x")
endif()

if(PROFILE_GEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-generate -fprofile-dir=${PROFILE_GEN_DIR}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate -fprofile-dir=${PROFILE_GEN_DIR}")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-use -fprofile-dir=${CMAKE_BINARY_DIR}/profile")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use -fprofile-dir=${CMAKE_BINARY_DIR}/profile")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
endif()

project(snes9x)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

find_package(SDL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Freetype REQUIRED)
find_package(Gettext REQUIRED)
find_library(Intl_LIBRARY "intl" DOC "libintl libraries (if not in the C library)")
mark_as_advanced(Intl_LIBRARY)

set(SRC_FILES
	bsx.cpp c4.cpp c4emu.cpp cheats.cpp cheats2.cpp clip.cpp conffile.cpp
	controls.cpp cpu.cpp cpuexec.cpp cpuops.cpp crosshairs.cpp dma.cpp
	dsp.cpp dsp1.cpp dsp2.cpp dsp3.cpp dsp4.cpp fxinst.cpp fxemu.cpp gfx.cpp
	globals.cpp loadzip.cpp logger.cpp memmap.cpp movie.cpp obc1.cpp ppu.cpp
	stream.cpp sa1.cpp sa1cpu.cpp screenshot.cpp sdd1.cpp sdd1emu.cpp seta.cpp
	seta010.cpp seta011.cpp seta018.cpp snapshot.cpp snes9x.cpp spc7110.cpp
	srtc.cpp tile.cpp tileimpl-n1x1.cpp tileimpl-n2x1.cpp tileimpl-h2x1.cpp
	statemanager.cpp sha256.cpp bml.cpp

	apu/apu.cpp apu/SNES_SPC.cpp apu/SNES_SPC_misc.cpp
	apu/SNES_SPC_state.cpp apu/SPC_DSP.cpp apu/SPC_Filter.cpp

	filter/2xsai.cpp filter/blit.cpp filter/epx.cpp filter/hq2x.cpp
	filter/snes_ntsc.c

	unzip/ioapi.c unzip/unzip.c

	jma/7zlzma.cpp jma/crc32.cpp jma/iiostrm.cpp jma/inbyte.cpp
	jma/jma.cpp jma/lzma.cpp jma/lzmadec.cpp jma/s9x-jma.cpp jma/winout.cpp

	sdl/input.cpp sdl/sound.cpp sdl/video.cpp sdl/ttf.cpp sdl/util.cpp
	sdl/main.cpp sdl/menu.cpp sdl/i18n.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

add_executable(${PROJECT_NAME} ${SRC_FILES})

if(MINGW)
    set(EXTRA_FLAGS __USE_MINGW_ANSI_STDIO=1)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	ZLIB UNZIP_SUPPORT JMA_SUPPORT ${EXTRA_FLAGS})

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/filter
	${CMAKE_CURRENT_SOURCE_DIR}/apu ${CMAKE_CURRENT_SOURCE_DIR}/unzip
	${FREETYPE_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${FREETYPE_LIBRARIES}
	${SDL_LIBRARY} ${ZLIB_LIBRARIES} ${Intl_LIBRARY})

file(STRINGS ${CMAKE_SOURCE_DIR}/translations/locales TRANSLATIONS)
set(mo_files)
foreach(trans ${TRANSLATIONS})
  string(REPLACE " " ";" transl "${trans}")
  list(LENGTH transl len)
  if(len EQUAL 0)
    continue()
  endif()
  list(GET transl 0 locale)
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/locale/${locale}/LC_MESSAGES/snes9x.mo"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/locale/${locale}/LC_MESSAGES"
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
      -o "${CMAKE_BINARY_DIR}/locale/${locale}/LC_MESSAGES/snes9x.mo"
      "${CMAKE_SOURCE_DIR}/translations/${locale}.po"
    DEPENDS "${CMAKE_SOURCE_DIR}/translations/${locale}.po"
    )
  set(mo_files ${mo_files} "${CMAKE_BINARY_DIR}/locale/${locale}/LC_MESSAGES/snes9x.mo")
endforeach()

add_custom_target(update-mo ALL DEPENDS ${mo_files})
