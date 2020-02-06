cmake_minimum_required(VERSION 3.10)

include(CMake/out_of_tree.cmake)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

option(DEBUG "Enable debug mode in engine" OFF)
option(APOLLOBLIT "Enable AMMX blitting for Vampire Accelerators" ON)
option(DEBUG_RELEASE "Enable options for debug release" OFF)

if(DEBUG_RELEASE)
	set(CMAKE_BUILD_TYPE "Debug")
	set(DEBUG ON)
endif()

include(CMake/git.cmake)
get_git_tag(GIT_TAG)
if(NOT CMAKE_BUILD_TYPE MATCHES "Release")
	get_git_commit_hash(GIT_COMMIT_HASH)
endif()

project(SDL1.2
		VERSION ${GIT_TAG}
		LANGUAGES C ASM)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME_LOWER)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")

set(LIBNAME "SDL")

# Make version variables
string(REPLACE "." ";" VERSION_LIST ${GIT_TAG})
list(GET VERSION_LIST 0 SDL_MAJOR_VERSION)
list(GET VERSION_LIST 1 SDL_MINOR_VERSION)
list(GET VERSION_LIST 2 SDL_PATCHLEVEL)

message(STATUS "SDL version: ${SDL_MAJOR_VERSION}.${SDL_MINOR_VERSION}.${SDL_PATCHLEVEL}")

# Build date Information
string(TIMESTAMP VER_YEAR "%Y")
string(TIMESTAMP VER_MONTH "%m")
string(TIMESTAMP VER_DAY "%d")
string(REGEX REPLACE "^0+" "" VER_MONTH "${VER_MONTH}")
string(REGEX REPLACE "^0+" "" VER_DAY "${VER_DAY}")

# Set Amiga version
set(AMIGA_VERSION "${SDL_MAJOR_VERSION}${SDL_MINOR_VERSION}.${SDL_PATCHLEVEL}")

# Final build filename, for example "milkytracker-1.02.00-win32.zip"
set(BUILD_FILENAME "SDL-${GIT_TAG}-${SYSTEM_NAME_LOWER}")

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME_LOWER})
set(CPACK_PACKAGE_VENDOR "AmigaDev Team")
set(CPACK_PACKAGE_CONTACT "support@amigadev.com")
set(CPACK_PACKAGE_FILE_NAME ${BUILD_FILENAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${SDL_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${SDL_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${SDL_PATCHLEVEL})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SDL1.2")
include(CPack)

# Generate version header from the above
configure_file(
		${PROJECT_SOURCE_DIR}/include/SDL/SDL_version.h.in
		${PROJECT_BINARY_DIR}/include/SDL/SDL_version.h
)

configure_file(
		${PROJECT_SOURCE_DIR}/include/SDL/SDL_amigaversion.h.in
		${PROJECT_BINARY_DIR}/include/SDL/SDL_amigaversion.h
)

set(CMAKE_CXX_STANDARD 14)



add_library(${LIBNAME}
		STATIC
			# MAIN
			${PROJECT_SOURCE_DIR}/SDL.c
			${PROJECT_SOURCE_DIR}/SDL_cdrom.c
			${PROJECT_SOURCE_DIR}/SDL_syscdrom.c
			${PROJECT_SOURCE_DIR}/SDL_error.c
			${PROJECT_SOURCE_DIR}/SDL_fatal.c
			${PROJECT_SOURCE_DIR}/SDL_lutstub.s

			# AUDIO
			${PROJECT_SOURCE_DIR}/audio/SDL_audio.c
			${PROJECT_SOURCE_DIR}/audio/SDL_audiocvt.c
			${PROJECT_SOURCE_DIR}/audio/SDL_mixer.c
			${PROJECT_SOURCE_DIR}/audio/SDL_wave.c
			${PROJECT_SOURCE_DIR}/audio/dummy/SDL_dummyaudio.c
			${PROJECT_SOURCE_DIR}/audio/amigaos/SDL_ahiaudio.c

			# VIDEO
			${PROJECT_SOURCE_DIR}/video/SDL_RLEaccel.c
			${PROJECT_SOURCE_DIR}/video/SDL_blit.c
			${PROJECT_SOURCE_DIR}/video/SDL_blit_0.c
			${PROJECT_SOURCE_DIR}/video/SDL_blit_1.c
			${PROJECT_SOURCE_DIR}/video/SDL_blit_A.c
			${PROJECT_SOURCE_DIR}/video/SDL_blit_N.c
			${PROJECT_SOURCE_DIR}/video/SDL_bmp.c
			${PROJECT_SOURCE_DIR}/video/SDL_cursor.c
			${PROJECT_SOURCE_DIR}/video/SDL_pixels.c
			${PROJECT_SOURCE_DIR}/video/SDL_surface.c
			${PROJECT_SOURCE_DIR}/video/SDL_stretch.c
			${PROJECT_SOURCE_DIR}/video/SDL_yuv.c
			${PROJECT_SOURCE_DIR}/video/SDL_yuv_sw.c
			${PROJECT_SOURCE_DIR}/video/SDL_video.c
			${PROJECT_SOURCE_DIR}/video/SDL_gamma.c

			# CYBERGRAPHICS
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxvideo.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxmodes.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgximage.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_amigaevents.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_amigamouse.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxgl.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxwm.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxyuv.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxaccel.c
			${PROJECT_SOURCE_DIR}/video/cybergfx/SDL_cgxgl_wrapper.c

			# TIMER
			${PROJECT_SOURCE_DIR}/timer/amigaos/SDL_systimer.c
			${PROJECT_SOURCE_DIR}/timer/SDL_timer.c

			# JOYSTICK
			${PROJECT_SOURCE_DIR}/joystick/SDL_joystick.c
			${PROJECT_SOURCE_DIR}/joystick/SDL_sysjoystick.c

			# EVENTS
			${PROJECT_SOURCE_DIR}/events/SDL_quit.c
			${PROJECT_SOURCE_DIR}/events/SDL_active.c
			${PROJECT_SOURCE_DIR}/events/SDL_keyboard.c
			${PROJECT_SOURCE_DIR}/events/SDL_mouse.c
			${PROJECT_SOURCE_DIR}/events/SDL_resize.c
			${PROJECT_SOURCE_DIR}/events/SDL_events.c

			# FILE
			${PROJECT_SOURCE_DIR}/file/SDL_rwops.c

			# THREADS
			${PROJECT_SOURCE_DIR}/thread/amigaos/SDL_sysmutex.c
			${PROJECT_SOURCE_DIR}/thread/amigaos/SDL_syssem.c
			${PROJECT_SOURCE_DIR}/thread/amigaos/SDL_systhread.c
			${PROJECT_SOURCE_DIR}/thread/amigaos/SDL_thread.c
			${PROJECT_SOURCE_DIR}/thread/amigaos/SDL_syscond.c


			# STDLIB
			${PROJECT_SOURCE_DIR}/stdlib/SDL_stdlib.c
			${PROJECT_SOURCE_DIR}/stdlib/SDL_string.c
			${PROJECT_SOURCE_DIR}/stdlib/SDL_malloc.c
			${PROJECT_SOURCE_DIR}/stdlib/SDL_getenv.c)

target_include_directories(${LIBNAME}
		PRIVATE
			${PROJECT_SOURCE_DIR}/
			${PROJECT_SOURCE_DIR}/audio
			${PROJECT_SOURCE_DIR}/dummy
			${PROJECT_SOURCE_DIR}/audio/amigaos
			${PROJECT_SOURCE_DIR}/events
			${PROJECT_SOURCE_DIR}/include
			${PROJECT_SOURCE_DIR}/include/GL
			${PROJECT_SOURCE_DIR}/include/SDL
			${PROJECT_BINARY_DIR}/include/SDL
			${PROJECT_SOURCE_DIR}/joystick
			${PROJECT_SOURCE_DIR}/main/amigaos
			${PROJECT_SOURCE_DIR}/thread
			${PROJECT_SOURCE_DIR}/thread/amigaos
			${PROJECT_SOURCE_DIR}/thread/pth
			${PROJECT_SOURCE_DIR}/thread/pthread
			${PROJECT_SOURCE_DIR}/timer
			${PROJECT_SOURCE_DIR}/video
			${PROJECT_SOURCE_DIR}/video/cybergfx
			${PROJECT_SOURCE_DIR}/video/apollo
			${PROJECT_SOURCE_DIR}/video/dummy)

target_compile_definitions(${LIBNAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:NOIXEMUL _HAVE_STDINT_H>)

if(NOT DEBUG)
	target_compile_options(${LIBNAME} PUBLIC $<$<COMPILE_LANGUAGE:C>:-O3 -ffast-math>)
endif()

if(AMIGA)
	if(NOT DEBUG)
		target_compile_definitions(${LIBNAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:NO_AMIGADEBUG>)
	endif()

	if(APOLLOBLIT)
		target_compile_definitions(${LIBNAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:APOLLO_BLIT>)
		target_compile_options(${LIBNAME} PRIVATE -I${CMAKE_SYSROOT}/m68k-amigaos/ndk-include)
		target_include_directories(${LIBNAME} PUBLIC video/apollo)
		enable_language(ASM-VASM)
		target_sources(${LIBNAME}
				PRIVATE
					# APOLLO AMMX
					${PROJECT_SOURCE_DIR}/video/apollo/blitapollo.asm
					${PROJECT_SOURCE_DIR}/video/apollo/apolloammxenable.asm
					${PROJECT_SOURCE_DIR}/video/apollo/colorkeyapollo.asm)

	endif()
endif()


file(GLOB INCLUDE_FILES ${PROJECT_SOURCE_DIR}/include/SDL/*.h)
file(GLOB BIN_INCLUDE_FILES ${PROJECT_BINARY_DIR}/include/SDL/*.h)
list(APPEND INCLUDE_FILES ${BIN_INCLUDE_FILES})

file(GLOB GL_INCLUDE_FILES ${PROJECT_SOURCE_DIR}/include/GL/*.h)

install(FILES ${PROJECT_BINARY_DIR}/libSDL.a DESTINATION "lib${LIB_SUFFIX}")
install(FILES ${INCLUDE_FILES} DESTINATION include/SDL)
install(FILES ${GL_INCLUDE_FILES} DESTINATION include/GL)

install(FILES
		README.md
		README-SDL.md
		README-AMIGA.md
		LICENSE
		CREDITS
		COPYING
		BUGS
		DESTINATION ./)