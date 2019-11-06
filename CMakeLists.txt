cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if("${MODEL}" MATCHES "gcw0")
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/mips32-linux-gcc.cmake")
endif()

project(snes9x)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

find_package(SDL REQUIRED)
find_package(ZLIB REQUIRED)

set(SRC_FILES
	apu/apu.cpp
	apu/SNES_SPC.cpp
	apu/SNES_SPC_misc.cpp
	apu/SNES_SPC_state.cpp
	apu/SPC_DSP.cpp
	apu/SPC_Filter.cpp
	bsx.cpp
	c4.cpp
	c4emu.cpp
	cheats.cpp
	cheats2.cpp
	clip.cpp
	conffile.cpp
	controls.cpp
	cpu.cpp
	cpuexec.cpp
	cpuops.cpp
	crosshairs.cpp
	dma.cpp
	dsp.cpp
	dsp1.cpp
	dsp2.cpp
	dsp3.cpp
	dsp4.cpp
	fxinst.cpp
	fxemu.cpp
	gfx.cpp
	globals.cpp
	logger.cpp
	memmap.cpp
	movie.cpp
	obc1.cpp
	ppu.cpp
	stream.cpp
	sa1.cpp
	sa1cpu.cpp
	screenshot.cpp
	sdd1.cpp
	sdd1emu.cpp
	seta.cpp
	seta010.cpp
	seta011.cpp
	seta018.cpp
	snapshot.cpp
	snes9x.cpp
	spc7110.cpp
	srtc.cpp
	tile.cpp
	tileimpl-n1x1.cpp
	tileimpl-n2x1.cpp
	tileimpl-h2x1.cpp
	filter/2xsai.cpp
	filter/blit.cpp
	filter/epx.cpp
	filter/hq2x.cpp
	filter/snes_ntsc.c
	statemanager.cpp
	sha256.cpp
	bml.cpp
	sdl/main.cpp sdl/sound.cpp sdl/video.cpp sdl/sound.h sdl/video.h sdl/util.cpp sdl/util.h sdl/control.cpp sdl/control.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(${PROJECT_NAME} ${SRC_FILES})

if(MINGW)
    set(EXTRA_FLAGS __USE_MINGW_ANSI_STDIO=1)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE ${EXTRA_FLAGS})
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/filter
	${CMAKE_CURRENT_SOURCE_DIR}/apu ${CMAKE_CURRENT_SOURCE_DIR}/apu/bapu
	${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL_LIBRARY} ${ZLIB_LIBRARIES})
