/*
    include - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"


#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>


#include "amigaos_video_AGA.h"
#include "amigaos_events.h"
#include "amigaos_mouse.h"


#include "amiga_c2p_aga.h"


// PAL modes!
static SDL_Rect
		RECT_640x512 = { 0, 0, 640, 512 },
		RECT_640x480 = { 0, 0, 640, 480 },
		RECT_640x256 = { 0, 0, 640, 256 },
		RECT_320x256 = { 0, 0, 320, 256 },
		RECT_320x240 = { 0, 0, 320, 240 },
		RECT_320x200 = { 0, 0, 320, 200 };

static SDL_Rect *vid_modes[] = {
		&RECT_640x512,
		&RECT_640x480,
		&RECT_640x256,
		&RECT_320x256,
		&RECT_320x240,
		&RECT_320x200,
		NULL
};


static struct Window *_hardwareWindow = NULL;
static struct Screen *_hardwareScreen = NULL;

// Hardware double buffering.
static struct ScreenBuffer *_hardwareScreenBuffer[2];
static BYTE _currentScreenBuffer = 0;


// AGA C2P.
static void *c2p[2] = { NULL, NULL };

// Palette data
static SDL_Color *_currentPalette;
static int _paletteDirtyStart, _paletteDirtyEnd;


// These settings are for PAL HIGH RES!
#define AGA_VIDEO_DEPTH         8
#define AGA_MAX_VIDEO_WIDTH     640
#define AGA_MAX_VIDEO_HEIGHT    512


static int AGA_VideoInit(SDL_PixelFormat *vformat) {
#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("[AGA_VideoInit]: START\n");
#endif

	/* Set the screen depth */
	vformat->BitsPerPixel = AGA_VIDEO_DEPTH;

	vformat->BytesPerPixel = 1;

	// Not used for 8 bit screens.
	vformat->Rmask = 0;
	vformat->Gmask = 0;
	vformat->Bmask = 0;

	// allocate palette storage
	_currentPalette = (SDL_Color *)SDL_calloc(256, sizeof(SDL_Color));

	_paletteDirtyStart = 256;
	_paletteDirtyEnd = 0;

	/* We're done! */
	return (0);
}

static SDL_Rect **AGA_ListModes(SDL_PixelFormat *format, Uint32 flags) {
	switch ( format->BitsPerPixel ) {
		case 8:
			return vid_modes;
		default:
			return NULL;
	}
}


static void AGA_ShutdownMode(void) {
	if ( _hardwareWindow ) {
		ClearPointer(_hardwareWindow);
		CloseWindow(_hardwareWindow);
		_hardwareWindow = NULL;
	}

	if ( _hardwareScreen ) {
		if ( _hardwareScreenBuffer[0] ) {
			ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
			WaitTOF();
			WaitTOF();
			FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
			_hardwareScreenBuffer[0] = NULL;
		}

		if ( _hardwareScreenBuffer[1] ) {
			FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[1]);
			_hardwareScreenBuffer[1] = NULL;
		}

		// Only close the screen if we actaully opened it ;)
		if ( current_video->hidden->fullScreen ) {
			CloseScreen(_hardwareScreen);
		}

		_hardwareScreen = NULL;
	}


	if ( c2p[0] ) {
		c2p8_deinit_stub(c2p[0]);
		c2p[0] = NULL;
	}

	if ( c2p[1] ) {
		c2p8_deinit_stub(c2p[1]);
		c2p[1] = NULL;
	}

	// Clear the hidden pointers.
	current_video->hidden->hardwareScreen = NULL;
	current_video->hidden->hardwareWindow = NULL;
	current_video->hidden->fullScreen = 0;
}

static struct Screen *CreateHardwareScreen(ULONG modeId, int width, int height, int depth) {

	// Create the hardware screen.
	return OpenScreenTags(NULL,
						  SA_Depth, depth,
						  SA_DisplayID, modeId,
						  SA_Top, 0,
						  SA_Left, 0,
						  SA_Width, width,
						  SA_Height, height,
						  SA_Type, CUSTOMSCREEN,
						  SA_Quiet, TRUE,
						  SA_ShowTitle, FALSE,
						  SA_Draggable, FALSE,
						  SA_Exclusive, TRUE,
						  SA_AutoScroll, FALSE,
						  TAG_END);
}

static struct Window *CreateHardwareWindow(int width, int height) {

	return OpenWindowTags(NULL,
						  WA_Left, 0,
						  WA_Top, 0,
						  WA_InnerWidth, width,
						  WA_InnerHeight, height,
						  WA_PubScreen, (ULONG)_hardwareScreen,
						  WA_Backdrop, FALSE,
						  WA_Borderless, FALSE,
						  WA_Title, (ULONG)"include is Alive!",
						  WA_DragBar, TRUE,
						  WA_CloseGadget, TRUE,
						  WA_SizeGadget, TRUE,
						  WA_Activate, TRUE,
						  WA_SimpleRefresh, TRUE,
						  WA_NoCareRefresh, TRUE,
						  WA_ReportMouse, TRUE,
						  WA_RMBTrap, TRUE,
						  WA_IDCMP, IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_NEWSIZE,
						  WA_GimmeZeroZero, TRUE,
						  TAG_END);
}

static struct Window *CreateFullScreenHardwareWindow(int width, int height) {

	return OpenWindowTags(NULL,
						  WA_Left, 0,
						  WA_Top, 0,
						  WA_Width, width,
						  WA_Height, height,
						  WA_CustomScreen, (ULONG)_hardwareScreen,
						  WA_Backdrop, TRUE,
						  WA_Borderless, TRUE,
						  WA_Activate, TRUE,
						  WA_SimpleRefresh, TRUE,
						  WA_NoCareRefresh, TRUE,
						  WA_ReportMouse, TRUE,
						  WA_RMBTrap, TRUE,
						  WA_IDCMP, IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE | IDCMP_MOUSEBUTTONS,
						  TAG_END);
}


static SDL_Surface *AGA_SetVideoMode(SDL_Surface *current, int width, int height, int bpp, Uint32 flags) {
#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("AGA_SetVideoMode(width=%d, height=%d, bpp=%d)\n", width, height, bpp);
#endif


#ifdef DEBUG_VIDEO
	if (flags&SDL_HWPALETTE) {
		SDL_LogDebugMsgchar("SDL_HWPALETTE requested\n");
	} else {
		SDL_LogDebugMsgchar("SDL_HWPALETTE NOT requested\n");
	}

	if (flags&SDL_DOUBLEBUF) {
		SDL_LogDebugMsgchar("AGA_SetVideoMode() - SDL_DOUBLEBUF requested\n");
	} else {
		SDL_LogDebugMsgchar("SDL_DOUBLEBUF NOT requested\n");
	}

	if (flags&SDL_FULLSCREEN) {
		SDL_LogDebugMsgchar("AGA_SetVideoMode() - SDL_FULLSCREEN requested\n");
	} else {
		SDL_LogDebugMsgchar("SDL_FULLSCREEN NOT requested\n");
	}

	if (flags&SDL_RESIZABLE) {
		SDL_LogDebugMsgchar("AGA_SetVideoMode() - SDL_RESIZABLE requested\n");
	} else {
		SDL_LogDebugMsgchar("SDL_RESIZABLE NOT requested\n");
	}
#endif



	// Check args.
	if ( bpp != AGA_VIDEO_DEPTH ) {
		SDL_SetError("Invalid depth requested, only %d bit mode supported", AGA_VIDEO_DEPTH);
		return NULL;
	}

	if ( flags & SDL_FULLSCREEN ) {
		if ( width > AGA_MAX_VIDEO_WIDTH ) {
			SDL_SetError("Requested width cannot be greater than %d", AGA_MAX_VIDEO_WIDTH);
			return NULL;
		}

		if ( height > AGA_MAX_VIDEO_HEIGHT ) {
			SDL_SetError("Requested height cannot be greater than %d", AGA_MAX_VIDEO_HEIGHT);
			return NULL;
		}
	} else {
		_hardwareScreen = LockPubScreen(NULL);
		if ( !_hardwareScreen ) {
			SDL_SetError("Couldn't lock the display");
			return NULL;
		}

		UnlockPubScreen(NULL, _hardwareScreen);

		// Check that the current WB Screen depth matches the requested bit depth.
		if ( _hardwareScreen->RastPort.BitMap->Depth != AGA_VIDEO_DEPTH ) {
			SDL_SetError("Invalid Workbench Screen depth, only %d bit mode supported", AGA_VIDEO_DEPTH);
			return NULL;
		}

		// And that it's big enough.
		if ( width > _hardwareScreen->Width ) {
			SDL_SetError("Requested width cannot be greater than the width of the Workbench Screen");
			return NULL;
		}

		if ( height > _hardwareScreen->Height ) {
			SDL_SetError("Requested height cannot be greater than the height of the Workbench Screen");
			return NULL;
		}
	}




	// Free any existing mode.
	AGA_ShutdownMode();


	if ( flags & SDL_FULLSCREEN ) {
		ULONG modeId = INVALID_ID;
		DisplayInfoHandle handle;
		struct DisplayInfo dispinfo;
		struct DimensionInfo dimsinfo;


		// Automatically choose the best mode.
		modeId = BestModeID(BIDTAG_NominalWidth, width,
							BIDTAG_NominalHeight, height,
							BIDTAG_DesiredWidth, width,
							BIDTAG_DesiredHeight, height,
							BIDTAG_Depth, bpp,
							BIDTAG_MonitorID, PAL_MONITOR_ID,
							TAG_END);


		// Verify the mode choosen.
		if ( modeId != INVALID_ID ) {
			if ((handle = FindDisplayInfo(modeId)) == NULL ) {
				SDL_SetError("Couldn't find Display Info for requested mode");
				return NULL;
			}

			if ( GetDisplayInfoData(handle, (UBYTE * ) & dispinfo, sizeof(dispinfo), DTAG_DISP, 0) == 0 ) {
				SDL_SetError("Couldn't get Display Info Data for requested mode");
				return NULL;
			}

			if ( GetDisplayInfoData(handle, (UBYTE * ) & dimsinfo, sizeof(dimsinfo), DTAG_DIMS, 0) == 0 ) {
				SDL_SetError("Couldn't get Display Info Data for requested mode");
				return NULL;
			}

			if ( dimsinfo.MaxDepth != bpp ) {
				modeId = INVALID_ID;
			}

			if ((dimsinfo.Nominal.MaxX + 1) != width ) {
				modeId = INVALID_ID;
			}

			if ((dimsinfo.Nominal.MaxY + 1) < height ) {
				modeId = INVALID_ID;
			}
		}

		if ( modeId == INVALID_ID ) {
			SDL_SetError("Couldn't find a Screen Mode for requested mode");
			return NULL;
		}

		// Create the hardware screen.
		_hardwareScreen = CreateHardwareScreen(modeId, width, height, bpp);
		if ( !_hardwareScreen ) {
			SDL_SetError("Couldn't create a Hardware Screen for requested mode");
			return NULL;
		}



		// See if we need to set up double buffering.
		if ( flags & SDL_DOUBLEBUF ) {
#ifdef DEBUG_VIDEO
			SDL_LogDebugMsgchar("Setting up double buffering for a full screen mode\n");
#endif
			_hardwareScreenBuffer[0] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);
			_hardwareScreenBuffer[1] = AllocScreenBuffer(_hardwareScreen, NULL, 0);

			c2p[0] = c2p8_reloc_stub(_hardwareScreenBuffer[0]->sb_BitMap);
			c2p[1] = c2p8_reloc_stub(_hardwareScreenBuffer[1]->sb_BitMap);

			_currentScreenBuffer = 1;


			flags = (SDL_FULLSCREEN | SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF);
		} else {
#ifdef DEBUG_VIDEO
			SDL_LogDebugMsgchar("Not using double buffering for a full screen mode\n");
#endif
			_hardwareScreenBuffer[0] = NULL;
			_hardwareScreenBuffer[1] = NULL;

			c2p[0] = NULL;
			c2p[1] = NULL;

			// Overwrite the flags - maybe not cool?
			flags = (SDL_FULLSCREEN | SDL_SWSURFACE | SDL_HWPALETTE);
		}


		// Create the hardware window.
		_hardwareWindow = CreateFullScreenHardwareWindow(width, height);
		if ( !_hardwareWindow ) {
			SDL_SetError("Couldn't create a Hardware Window for requested mode");
			return NULL;
		}

		// Set the hidden pointers.
		current_video->hidden->fullScreen = TRUE;
		current_video->hidden->hardwareScreen = _hardwareScreen;
		current_video->hidden->hardwareWindow = _hardwareWindow;
	} else {
#ifdef DEBUG_VIDEO
		SDL_LogDebugMsgchar("Setting up a Windowed mode\n");
#endif


		//SDL_HWPALETTE - think this is OK
		//SDL_HWSURFACE - think this is OK

		// Overwrite the flags - maybe not cool?
		flags = (SDL_SWSURFACE | SDL_HWPALETTE);


		// Create the hardware window.
		_hardwareWindow = CreateHardwareWindow(width, height);
		if ( !_hardwareWindow ) {
			SDL_SetError("Couldn't create a Hardware Window for requested mode");
			return NULL;
		}

		// Set the hidden pointers.
		current_video->hidden->fullScreen = FALSE;
		current_video->hidden->hardwareScreen = _hardwareScreen;
		current_video->hidden->hardwareWindow = _hardwareWindow;
	}


	if ( current ) {
		SDL_FreeSurface(current);
		current = NULL;
	}

	current = SDL_CreateRGBSurface(flags, width, height, bpp, 0, 0, 0, 0);


#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("Setting current_video\n");
#endif



	// Set the default grab mode.
	//current_video->input_grab = SDL_GRAB_OFF;

	// Update info to match the new mode.
	current_video->info.vfmt = current->format;
	current_video->info.current_w = current->w;
	current_video->info.current_h = current->h;
	current_video->info.video_mem = 2048; // AGA.


	/* We're done */
#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("Finished setting mode %dx%d\n", width, height);
#endif

	return current;
}


static int AGA_ToggleFullScreen(int on) {
	// TODO
}

static void AGA_updatePalette() {

	int i, numberOfEntries;
	ULONG _agaPalette[256 * 3 + 2];

#ifdef DEBUG_PALETTE
	SDL_LogDebugMsgchar ("AGA_updatePalette()\n");

	SDL_LogDebugMsgchar("AGA_updatePalette() - _paletteDirtyStart = %d\n", _paletteDirtyStart);
	SDL_LogDebugMsgchar("AGA_updatePalette() - _paletteDirtyEnd = %d\n", _paletteDirtyEnd);
#endif

	numberOfEntries = (_paletteDirtyEnd - _paletteDirtyStart);

	for ( i = 0; i < numberOfEntries; i++ ) {
		_agaPalette[i * 3 + 1] = _currentPalette[_paletteDirtyStart + i].r << 24;
		_agaPalette[i * 3 + 2] = _currentPalette[_paletteDirtyStart + i].g << 24;
		_agaPalette[i * 3 + 3] = _currentPalette[_paletteDirtyStart + i].b << 24;
	}

	_agaPalette[0] = (numberOfEntries << 16) + _paletteDirtyStart;

	// Terminator: NEEDED
	_agaPalette[((numberOfEntries * 3) + 1)] = 0x00000000;

	LoadRGB32(&_hardwareScreen->ViewPort, _agaPalette);


	// Reset.
	_paletteDirtyStart = 256;
	_paletteDirtyEnd = 0;
}


static int AGA_AllocHWSurface(SDL_Surface *surface) {
	return -1;
}

static void AGA_FreeHWSurface(SDL_Surface *surface) {

}

static int AGA_LockHWSurface(SDL_Surface *surface) {
	return -1;
}

static void AGA_UnlockHWSurface(SDL_Surface *surface) {

}

// Note - Not really a HW surface!
static int AGA_FlipHWSurface(SDL_Surface *surface) {
#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("AGA_FlipHWSurface()\n");
#endif

	// Check whether the palette was changed.
	if ( _paletteDirtyEnd != 0 ) {
		AGA_updatePalette();
	}

	c2p8_stub(c2p[_currentScreenBuffer], _hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap, (UBYTE * )SDL_VideoSurface->pixels, (SDL_VideoSurface->w * SDL_VideoSurface->h));

	if ( ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[_currentScreenBuffer])) {
		// Flip.
		_currentScreenBuffer = _currentScreenBuffer ^ 1;
	}

	return 0; // Strange but true!
}

static void AGA_UpdateRects(int numrects, SDL_Rect *rects) {
	int i;
	Uint16 dirtyStart, dirtyEnd;
	UBYTE *src;


#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("AGA_UpdateRects()\n");
#endif

	// Seed.
	dirtyStart = SDL_VideoSurface->h;
	dirtyEnd = 0;


	// Find dirty strip extents.
	for ( i = 0; i < numrects; i++ ) {
		SDL_Rect *r = rects + i;
		if ( !r ) {
			continue;
		}

		if ( r->y < dirtyStart ) {
			dirtyStart = r->y;
		}

		if ((r->y + r->h) > dirtyEnd ) {
			dirtyEnd = (r->y + r->h);
		}
	}


	// render straight to screen, no double buffering.
	src = (UBYTE * )SDL_VideoSurface->pixels + (dirtyStart * SDL_VideoSurface->w);

	// Check whether the palette was changed.
	if ( _paletteDirtyEnd != 0 ) {
		AGA_updatePalette();
	}

	WaitTOF();


	//WriteChunkyPixels(rp,xstart,ystart,xstop,ystop,array,bytesperrow)
	//WriteChunkyPixels(_hardwareWindow->RPort, 0, dirtyStart, SDL_VideoSurface->w - 1, dirtyEnd - 1, src, SDL_VideoSurface->w);
	//WriteChunkyPixels(_hardwareWindow->RPort, 0, dirtyStart, SDL_VideoSurface->w - 1, dirtyEnd - 1, src, SDL_VideoSurface->w);


	//c2p1x1_8_c5_bm_stub(SDL_VideoSurface->w, (dirtyEnd - dirtyStart), 0, dirtyStart, src, (UBYTE*)_hardwareScreen->RastPort.BitMap);
	c2p1x1_8_c5_bm_stub(SDL_VideoSurface->w, (dirtyEnd - dirtyStart), 0, dirtyStart, src, (UBYTE *)_hardwareWindow->RPort->BitMap);
}

static int AGA_SetColors(int firstcolor, int ncolors, SDL_Color *colors) {
#ifdef DEBUG_PALETTE
	SDL_LogDebugMsgchar("AGA_SetColors()\n");

	SDL_LogDebugMsgchar("AGA_SetColors() - firstcolor = %d\n", firstcolor);
	SDL_LogDebugMsgchar("AGA_SetColors() - ncolors = %d\n", ncolors);
#endif

	SDL_memcpy(_currentPalette + firstcolor, colors, ncolors * sizeof(*colors));

	if ( firstcolor < _paletteDirtyStart ) {
		_paletteDirtyStart = firstcolor;
	}

	if ( firstcolor + ncolors > _paletteDirtyEnd ) {
		_paletteDirtyEnd = firstcolor + ncolors;
	}

	return 1;
}

/* Note:  If we are terminated, this could be called in the middle of
   another include video routine -- notably UpdateRects.
*/
static void AGA_VideoQuit() {
#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("AGA_VideoQuit()\n");
#endif

	// Free any existing mode.
	AGA_ShutdownMode();

	if ( current_video ) {
		// Clear the hidden pointers.
		current_video->hidden->hardwareScreen = NULL;
		current_video->hidden->hardwareWindow = NULL;

		if ( SDL_VideoSurface != NULL ) {
			SDL_free(SDL_VideoSurface->pixels);
			SDL_VideoSurface->pixels = NULL;
		}
	}

	if ( _currentPalette ) {
		SDL_free(_currentPalette);
		_currentPalette = NULL;
	}
}

// This function fills in the structure pointed to by info with window-manager specific information.
static int AGA_GetWMInfo(SDL_SysWMinfo *info) {
	if ( info ) {
		info->version.major = SDL_MAJOR_VERSION;
		info->version.minor = SDL_MINOR_VERSION;
		info->version.patch = SDL_PATCHLEVEL;

		return 1;
	} else {
		return -1;
	}
}


/* AGA driver bootstrap functions */
static int AGA_Available(void) {
	// TODO - see if this is an AGA machine!

	return 1;
}

static void AGA_DeleteDevice() {
	if ( current_video ) {
#ifdef DEBUG_VIDEO
		SDL_LogDebugMsgchar("[AGA_DeleteDevice]: current_video is not NULL\n");
#endif
		SDL_free(current_video->hidden);
		SDL_free(current_video);
	}
}

static SDL_VideoDevice *AGA_CreateDevice(int devindex) {
	SDL_VideoDevice *device;

#ifdef DEBUG_VIDEO
	SDL_LogDebugMsgchar("[AGA_CreateDevice]: devindex = %d\n", devindex);
#endif

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)SDL_malloc((sizeof *device->hidden));
	}
	if ((device == NULL) || (device->hidden == NULL)) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return (0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));


	/* Set the function pointers */
	device->VideoInit = AGA_VideoInit;
	device->ListModes = AGA_ListModes;
	device->SetVideoMode = AGA_SetVideoMode;
	device->ToggleFullScreen = AGA_ToggleFullScreen;
	device->CreateYUVOverlay = NULL;
	device->SetColors = AGA_SetColors;
	device->UpdateRects = AGA_UpdateRects;
	device->VideoQuit = AGA_VideoQuit;

	device->AllocHWSurface = AGA_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = AGA_LockHWSurface;
	device->UnlockHWSurface = AGA_UnlockHWSurface;
	device->FlipHWSurface = AGA_FlipHWSurface;
	device->FreeHWSurface = AGA_FreeHWSurface;

	device->UpdateMouse = NULL;
	device->GrabInput = NULL;

	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GetWMInfo = AGA_GetWMInfo;

	device->FreeWMCursor = amiga_FreeWMCursor;
	device->CreateWMCursor = amiga_CreateWMCursor;
	device->ShowWMCursor = amiga_ShowWMCursor;
	device->WarpWMCursor = amiga_WarpWMCursor;
	device->MoveWMCursor = NULL;
	device->CheckMouseMode = NULL;

	device->InitOSKeymap = amiga_InitOSKeymap;
	device->PumpEvents = amiga_PumpEvents;

	device->free = AGA_DeleteDevice;

	return device;
}

VideoBootStrap AGA_bootstrap = {
		"AGA", "AmigaOS AGA",
		AGA_Available, AGA_CreateDevice
};
