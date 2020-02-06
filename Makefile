# Makefile for gcc version of SDL
#
# changes:
#  18-Apr: _ApolloKeyRGB565toRGB565: disabled AMMX version of ColorKeying (for now, storem is not working in Gold2)
#  17-Nov: - fixed Shadow Surfaces (hopefully), they were effectively impossible
#            in the code base I got
#          - fixed ARGB32 (CGX code was assuming RGBA all the time)
#  12-Feb: - deleted redundant includes, now only SDL/ directory remains (as it should)

PREFX := /opt/m68k-amigaos/

CC := $(PREFX)/bin/m68k-amigaos-gcc
AS := $(PREFX)/bin/m68k-amigaos-as
AR := $(PREFX)/bin/m68k-amigaos-ar
LD := $(PREFX)/bin/m68k-amigaos-ld
RL := $(PREFX)/bin/m68k-amigaos-ranlib
VASM := $(PREFX)/bin/vasmm68k_mot

CPU := 68040

INCLUDES = IDIR=./include/SDL

GCCFLAGS = -I$(PREFX)/include -I./include/ -I./include/SDL -I./makefile-support \
		-O3 -fomit-frame-pointer -m$(CPU) -mhard-float -ffast-math -noixemul \
		-DNOIXEMUL -D_HAVE_STDINT_H
GLFLAGS = -DSHARED_LIB -lamiga
GCCFLAGS += -DNO_AMIGADEBUG
GLFLAGS  += -DNO_AMIGADEBUG

GOBJS = audio/SDL_audio.go audio/SDL_audiocvt.go audio/SDL_mixer.go audio/SDL_wave.go audio/amigaos/SDL_ahiaudio.go \
	SDL_error.go SDL_fatal.go video/SDL_RLEaccel.go video/SDL_blit.go video/SDL_blit_0.go \
	video/SDL_blit_1.go video/SDL_blit_A.go video/SDL_blit_N.go \
	video/SDL_bmp.go video/SDL_cursor.go video/SDL_pixels.go video/SDL_surface.go video/SDL_stretch.go \
	video/SDL_yuv.go video/SDL_yuv_sw.go video/SDL_video.go \
	timer/amigaos/SDL_systimer.go timer/SDL_timer.go joystick/SDL_joystick.go \
	joystick/SDL_sysjoystick.go SDL_cdrom.go SDL_syscdrom.go events/SDL_quit.go events/SDL_active.go \
	events/SDL_keyboard.go events/SDL_mouse.go events/SDL_resize.go file/SDL_rwops.go SDL.go \
	events/SDL_events.go thread/amigaos/SDL_sysmutex.go thread/amigaos/SDL_syssem.go thread/amigaos/SDL_systhread.go thread/amigaos/SDL_thread.go \
	thread/amigaos/SDL_syscond.go video/amigaos/SDL_cgxvideo.go video/amigaos/SDL_cgxmodes.go video/amigaos/SDL_cgximage.go video/amigaos/SDL_amigaevents.go \
	video/amigaos/SDL_amigamouse.go video/amigaos/SDL_cgxgl.go video/amigaos/SDL_cgxwm.go \
	video/amigaos/SDL_cgxyuv.go video/amigaos/SDL_cgxaccel.go video/amigaos/SDL_cgxgl_wrapper.go \
	video/SDL_gamma.go SDL_lutstub.ll stdlib/SDL_stdlib.go stdlib/SDL_string.go stdlib/SDL_malloc.go stdlib/SDL_getenv.go

#
# BEGIN APOLLO ASM SUPPORT
# ( build vasm: make CPU=m68k SYNTAX=mot )
#
VFLAGS = -devpac -I$(PREFX)/m68k-amigaos/ndk-include -Fhunk
GCCFLAGS += -DAPOLLO_BLIT -I./video/apollo
# -DAPOLLO_BLITDBG
GOBJS += video/apollo/blitapollo.ao video/apollo/apolloammxenable.ao video/apollo/colorkeyapollo.ao

%.ao: %.asm
	$(VASM) $(VFLAGS) -o $@ $*.asm
#
# END APOLLO ASM SUPPORT
#

%.go: %.c
	$(CC) $(GCCFLAGS) $(GCCDEFINES) -o $@ -c $*.c

%.ll: %.s
	$(AS) -m$(CPU) -o $@ $*.s

all: libSDL.a

libSDL.a: $(GOBJS)
	-rm -f libSDL.a
	$(AR) cru libSDL.a $(GOBJS)
	$(RL) libSDL.a

clean:
	-rm -f $(GOBJS)
