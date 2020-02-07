/******************************************************************************/
/*                                                                            */
/* includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include <exec/types.h>
#include <proto/exec.h>

#include "../../../mydebug.h"

/******************************************************************************/
/*                                                                            */
/* global declarations                                                        */
/*                                                                            */
/******************************************************************************/

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct Library *UtilityBase;

/******************************************************************************/
/*                                                                            */
/* user library initialization                                                */
/*                                                                            */
/* !!! CAUTION: This function may run in a forbidden state !!!                */
/*                                                                            */
/******************************************************************************/

int __saveds __asm

__UserLibInit(register __a6 struct Library *libbase) {
	/* required !!! */
	SysBase = *(struct ExecBase **)4;

	D(bug("***include.library init: %lx\n", libbase));

	if ( !(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39L))) {
		D(bug("unable to open dos.library\n"));
		return 1;
	}

	if ( !(UtilityBase = OpenLibrary("utility.library", 37L))) {
		D(bug("***unable to open utility.library.\n"));
		CloseLibrary((struct Library *)DOSBase);
		return 1;
	}

	return 0;
}

/******************************************************************************/
/*                                                                            */
/* user library cleanup                                                       */
/*                                                                            */
/* !!! CAUTION: This function runs in a forbidden state !!!                   */
/*                                                                            */
/******************************************************************************/

void __saveds __asm

__UserLibCleanup(register __a6 struct Library *myLib) {
	D(bug("***include.library cleanup\n"));
	CloseLibrary(UtilityBase);
	CloseLibrary((struct Library *)DOSBase);
}

/******************************************************************************/
/*                                                                            */
/* library dependent function(s)                                              */
/*                                                                            */
/******************************************************************************/

#define ADDTABL_0(x)               \
  __saveds __asm LIB##x(void)      \
  {                                \
    extern __stdargs long x(void); \
    return x();                    \
  }
#define ADDTABL_1(x, y)                           \
  __saveds __asm LIB##x(register __##y long arg1) \
  {                                               \
    extern __stdargs long x(long);                \
    return x(arg1);                               \
  }
#define ADDTABL_2(x, y, z)                                                  \
  __saveds __asm LIB##x(register __##y long arg1, register __##z long arg2) \
  {                                                                         \
    extern __stdargs long x(long, long);                                    \
    return x(arg1, arg2);                                                   \
  }
#define ADDTABL_3(x, y, z, t)                                                                         \
  __saveds __asm LIB##x(register __##y long arg1, register __##z long arg2, register __##t long arg3) \
  {                                                                                                   \
    extern __stdargs long x(long, long, long);                                                        \
    return x(arg1, arg2, arg3);                                                                       \
  }
#define ADDTABL_4(x, y, z, t, v)                                                                                                \
  __saveds __asm LIB##x(register __##y long arg1, register __##z long arg2, register __##t long arg3, register __##v long arg4) \
  {                                                                                                                             \
    extern __stdargs long x(long, long, long, long);                                                                            \
    return x(arg1, arg2, arg3, arg4);                                                                                           \
  }
#define ADDTABL_5(x, y, z, t, v, k)                                                                                                                       \
  __saveds __asm LIB##x(register __##y long arg1, register __##z long arg2, register __##t long arg3, register __##v long arg4, register __##k long arg5) \
  {                                                                                                                                                       \
    extern __stdargs long x(long, long, long, long, long);                                                                                                \
    return x(arg1, arg2, arg3, arg4, arg5);                                                                                                               \
  }

ADDTABL_1(SDL_Init, d0) /* One Argument in d0 */
ADDTABL_0(SDL_Quit)

ADDTABL_1(SDL_InitSubSystem, d0)

ADDTABL_1(SDL_QuitSubSystem, d0)

ADDTABL_1(SDL_WasInit, d0)

ADDTABL_0(SDL_GetError)

ADDTABL_2(SDL_RWFromFile, a0, d0)

ADDTABL_2(SDL_RWFromFP, a0, d0)

ADDTABL_2(SDL_RWFromMem, a0, d0)

// video

ADDTABL_2(SDL_LoadBMP_RW, a0, d0)

ADDTABL_3(SDL_SetColorKey, a0, d0, d1)

ADDTABL_1(SDL_DisplayFormat, a0)

ADDTABL_1(SDL_FreeSurface, a0)

ADDTABL_3(SDL_FillRect, a0, a1, d0)

ADDTABL_4(SDL_UpperBlit, a0, d0, a1, d1)

ADDTABL_1(SDL_Flip, a0)

ADDTABL_3(SDL_UpdateRects, a0, d0, a1)

ADDTABL_5(SDL_UpdateRect, a0, d0, d1, d2, d3)

ADDTABL_0(SDL_GetVideoInfo)

ADDTABL_4(SDL_SetVideoMode, d0, d1, d2, d3)

ADDTABL_4(SDL_MapRGB, a0, d0, d1, d2)

// timer
ADDTABL_0(SDL_GetTicks)

ADDTABL_1(SDL_Delay, d0)

// events
ADDTABL_1(SDL_PollEvent, a0)

ADDTABL_1(SDL_WaitEvent, a0)

ADDTABL_4(SDL_PeepEvents, a0, d0, d1, d2)

/******************************************************************************/
/*                                                                            */
/* endtable marker (required!)                                                */
/*                                                                            */
/******************************************************************************/

// ADDTABL_END()

void _XCEXIT(void) {
	D(bug("Warning include.library attempted exit, sleeping forever!\n"));
	Wait(0); // resto in wait forever
}

/******************************************************************************/
/*                                                                            */
/* end of simplelib.c                                                         */
/*                                                                            */
/******************************************************************************/
