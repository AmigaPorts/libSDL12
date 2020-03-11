/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/



#include "SDL_error.h"
#include "SDL_cdrom.h"
#include "../SDL_syscdrom.h"

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <devices/scsidisk.h>

#include <proto/exec.h>
#include <proto/dos.h>

#define MAX_DRIVES      32
#define TOC_SIZE        804
#define SCSI_INBUF_SIZE TOC_SIZE
#define SCSI_SENSE_SIZE 254

static const char *SDL_SYS_CDName(int drive);
static int SDL_SYS_CDOpen(int drive);
static int SDL_SYS_CDGetTOC(SDL_CD *cdrom);
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position);
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length);
static int SDL_SYS_CDPause(SDL_CD *cdrom);
static int SDL_SYS_CDResume(SDL_CD *cdrom);
static int SDL_SYS_CDStop(SDL_CD *cdrom);
static int SDL_SYS_CDEject(SDL_CD *cdrom);
static void SDL_SYS_CDClose(SDL_CD *cdrom);

struct amigacd
{
	TEXT    devname[20];
	TEXT    dosname[108];
	uint32  unit;
	uint32  unitflags;

	struct  MsgPort *port;
	struct  IOStdReq *req;
	struct  SCSICmd scsicmd;
	uint16 *inbuf;
	uint8  *sensebuf;
};

struct amigacd *cddrives[MAX_DRIVES];
extern int SDL_numcds;

BOOL finddup( CONST_STRPTR devname, uint32 devnamelen, uint32 unit )
{
	int32 i;
	
	for( i=0; i<MAX_DRIVES; i++ )
	{
		if( !cddrives[i] ) break;

		if( ( cddrives[i]->unit == unit ) &&
		    ( strncmp( cddrives[i]->devname, devname, devnamelen ) == 0 ) )
			return TRUE;
	}
	
	return FALSE;
}

int  SDL_SYS_CDInit(void)
{
	struct DosList *dlist;
	uint32 devcount;
	int32 i;
	
//	IExec->DebugPrintF( "AmigaOS4 CDInit called\n" );
	
	for( i=0; i<MAX_DRIVES; i++ )
		cddrives[i] = NULL;
	
	SDL_CDcaps.Name = SDL_SYS_CDName;
	SDL_CDcaps.Open = SDL_SYS_CDOpen;
	SDL_CDcaps.GetTOC = SDL_SYS_CDGetTOC;
	SDL_CDcaps.Status = SDL_SYS_CDStatus;
	SDL_CDcaps.Play = SDL_SYS_CDPlay;
	SDL_CDcaps.Pause = SDL_SYS_CDPause;
	SDL_CDcaps.Resume = SDL_SYS_CDResume;
	SDL_CDcaps.Stop = SDL_SYS_CDStop;
	SDL_CDcaps.Eject = SDL_SYS_CDEject;
	SDL_CDcaps.Close = SDL_SYS_CDClose;

	dlist = IDOS->LockDosList( LDF_DEVICES | LDF_READ );
	if( !dlist )
	{
//		IExec->DebugPrintF( "Fail 1\n" );
		return 0;
	}

	devcount = 0;
	while( ( dlist = IDOS->NextDosEntry( dlist, LDF_DEVICES | LDF_READ ) ) != NULL )
	{
		struct FileSysStartupMsg *startup;
		struct amigacd *entry;
		CONST_STRPTR devname, dosname;
		uint32 devnamelen, dosnamelen;

		if( devcount >= MAX_DRIVES ) break;

		startup = BADDR( dlist->dol_misc.dol_handler.dol_Startup );
		if( ( (uint32)startup <= 0x400 ) ||
		    ( ( startup->fssm_Unit & 0xff000000 ) != 0 ) ||
		    ( startup->fssm_Device == 0 ) ||
		    ( startup->fssm_Environ == 0 ) )
		{
//			IExec->DebugPrintF( "Fail 2\n" );
			continue;
		}

		if( ( devname = BADDR( startup->fssm_Device ) ) == NULL )
		{
//			IExec->DebugPrintF( "Fail 3\n" );
			continue;
		}

		if( ( devnamelen = *devname++ ) == 0 )
		{
//			IExec->DebugPrintF( "Fail 3\n" );
			continue;
		}

		if( finddup( devname, devnamelen, startup->fssm_Unit ) )
		{
//			IExec->DebugPrintF( "Fail 4\n" );
			continue;
		}

		dosname    = (CONST_STRPTR)BADDR(dlist->dol_Name);
		dosnamelen = *dosname++;
		
		// FIXME: Workaround for the a1parallel.device error requester
		if( ( strncmp( dosname, "PRT", dosnamelen ) == 0 ) ||
		    ( strncmp( dosname, "PAR", dosnamelen ) == 0 ) )
		  continue;

		entry = IExec->AllocVecTags( sizeof( struct amigacd ),
					AVT_ClearWithValue, 0,
					AVT_Type,           MEMF_SHARED,
					TAG_DONE );
		if( !entry )
		{
//			IExec->DebugPrintF( "Fail 5\n" );
			continue;
		}

		entry->unit			= startup->fssm_Unit;
		entry->unitflags	= startup->fssm_Flags;
		
		memcpy( entry->devname, devname, devnamelen ); entry->devname[devnamelen] = 0;
		memcpy( entry->dosname, dosname, dosnamelen ); entry->dosname[dosnamelen] = 0;
		
		entry->req      = NULL;
		entry->inbuf    = NULL;
		entry->sensebuf = NULL;
		
//		IExec->DebugPrintF( "[SDL_SYS_CDInit] Found '%s' (%s,%d)\n", entry->dosname, entry->devname, entry->unit );
		
		cddrives[devcount++] = entry;
	}

	IDOS->UnLockDosList( LDF_DEVICES | LDF_READ );

	if( devcount )
	{
		struct IOStdReq *req;
		struct SCSICmd  *scsicmd;
		struct MsgPort  *port;
		uint32 i;
		uint8 *inbuf, *sensebuf;
		BOOL  is_cdrom;

		port = IExec->AllocSysObjectTags( ASOT_PORT, TAG_DONE );
		if( port )
		{
			req = IExec->AllocSysObjectTags( ASOT_IOREQUEST,
					ASOIOR_Size,      sizeof( struct IOStdReq ),
					ASOIOR_ReplyPort, port,
					TAG_DONE );
			if( req )
			{
				if( ( inbuf = IExec->AllocVecTags( SCSI_INBUF_SIZE, TAG_DONE ) ) )
				{
					if( ( sensebuf = IExec->AllocVecTags( SCSI_SENSE_SIZE, TAG_DONE ) ) )
					{
						if( ( scsicmd = (struct SCSICmd *)IExec->AllocVecTags( sizeof( struct SCSICmd ), TAG_DONE ) ) )
						{
							is_cdrom = FALSE;
							for( i=0; i<MAX_DRIVES; i++ )
							{
								if( cddrives[i] == NULL )
									break;
	
//								IExec->DebugPrintF( "[SDL_SYS_CDInit] Testing '%s'...\n", cddrives[i]->dosname );
	
								is_cdrom = FALSE;
								if( IExec->OpenDevice( cddrives[i]->devname, cddrives[i]->unit, (struct IORequest *)req, cddrives[i]->unitflags ) == 0 )
								{
									static const uint8 cmd[] = { 0x12, 0, 0, 0, 254, 0 };
	
//									IExec->DebugPrintF( "Got device\n" );

									req->io_Command = HD_SCSICMD;
									req->io_Data    = (APTR)scsicmd;
									req->io_Length  = sizeof( struct SCSICmd );
									
									scsicmd->scsi_Data        = (uint16 *)inbuf;
									scsicmd->scsi_Length      = SCSI_INBUF_SIZE;
									scsicmd->scsi_SenseActual = 0;
									scsicmd->scsi_SenseData   = sensebuf;
									scsicmd->scsi_SenseLength = SCSI_SENSE_SIZE;
									scsicmd->scsi_CmdActual   = 0;
									scsicmd->scsi_Command     = (uint8 *)cmd;
									scsicmd->scsi_CmdLength   = sizeof( cmd );
									scsicmd->scsi_Flags       = SCSIF_READ | SCSIF_AUTOSENSE;
									
									IExec->DoIO( (struct IORequest *)req );
	
//									IExec->DebugPrintF( "Done IO\n" );
									
									if( scsicmd->scsi_Status == 0 && req->io_Error == 0 )
									{
//										IExec->DebugPrintF( "Status is chunky\n" );
										if( ((inbuf[0]&5)==5) && inbuf[1] & 0x80 )
											is_cdrom = TRUE;
									}
									
									IExec->CloseDevice( (struct IORequest *)req );
								}
								
								if( is_cdrom == FALSE )
								{
//									IExec->DebugPrintF( "[SDL_SYS_CDInit] Discounting '%s'\n", cddrives[i]->dosname );
									IExec->FreeVec( cddrives[i] );
									devcount--;
									cddrives[i] = cddrives[devcount];
									cddrives[devcount] = NULL;
									i--;
								}
							}
						
							IExec->FreeVec( scsicmd );
						}
						
						IExec->FreeVec( sensebuf );
					}
					
					IExec->FreeVec( inbuf );
				}
				
				IExec->FreeSysObject( ASOT_IOREQUEST, req );
			}
			IExec->FreeSysObject( ASOT_PORT, port );
		}
	}
	
	SDL_numcds = devcount;

	return 0;
}

void SDL_SYS_CDQuit(void)
{
	int32 i;

	for( i=0; i<MAX_DRIVES; i++ )
	{
		if( cddrives[i] )
		{
			if( cddrives[i]->req )
			{
				IExec->CloseDevice( (struct IORequest *)cddrives[i]->req );
				IExec->FreeSysObject( ASOT_IOREQUEST, cddrives[i]->req );
			}
		
			if( cddrives[i]->port )
				IExec->FreeSysObject( ASOT_PORT, cddrives[i]->port );
		
			if( cddrives[i]->inbuf )
				IExec->FreeVec( cddrives[i]->inbuf );
		
			if( cddrives[i]->sensebuf )
				IExec->FreeVec( cddrives[i]->sensebuf );

			IExec->FreeVec( cddrives[i] );
			cddrives[i] = NULL;
		}
	}
}

static const char *SDL_SYS_CDName(int drive)
{
	if( !cddrives[drive] ) return NULL;
	return cddrives[drive]->dosname;
}

static int SDL_SYS_CDOpen(int drive)
{
	cddrives[drive]->port = IExec->AllocSysObjectTags( ASOT_PORT, TAG_DONE );
	if( !cddrives[drive]->port ) return -1;
	
	cddrives[drive]->req = IExec->AllocSysObjectTags( ASOT_IOREQUEST,
								ASOIOR_Size,      sizeof( struct IOStdReq ),
								ASOIOR_ReplyPort, cddrives[drive]->port,
								TAG_DONE );
	if( !cddrives[drive]->req )
	{
		IExec->FreeSysObject( ASOT_PORT, cddrives[drive]->port );
		return -1;
	}

	cddrives[drive]->inbuf = IExec->AllocVecTags( SCSI_INBUF_SIZE, TAG_DONE );
	if( !cddrives[drive]->inbuf )
	{
		IExec->FreeSysObject( ASOT_PORT, cddrives[drive]->port );
		IExec->FreeSysObject( ASOT_IOREQUEST, cddrives[drive]->req );
		return -1;
	}

	cddrives[drive]->sensebuf = IExec->AllocVecTags( SCSI_SENSE_SIZE, TAG_DONE );
	if( !cddrives[drive]->sensebuf )
	{
		IExec->FreeSysObject( ASOT_PORT, cddrives[drive]->port );
		IExec->FreeSysObject( ASOT_IOREQUEST, cddrives[drive]->req );
		IExec->FreeVec( cddrives[drive]->inbuf );
		return -1;
	}

	if( IExec->OpenDevice( cddrives[drive]->devname, cddrives[drive]->unit, (struct IORequest *)cddrives[drive]->req, cddrives[drive]->unitflags ) != 0 )
	{
		IExec->FreeSysObject( ASOT_PORT, cddrives[drive]->port );
		IExec->FreeSysObject( ASOT_IOREQUEST, cddrives[drive]->req );
		IExec->FreeVec( cddrives[drive]->inbuf );
		IExec->FreeVec( cddrives[drive]->sensebuf );
		return -1;
	}
	
	cddrives[drive]->req->io_Command = HD_SCSICMD;
	cddrives[drive]->req->io_Data    = (APTR)&cddrives[drive]->scsicmd;
	cddrives[drive]->req->io_Length  = sizeof( struct SCSICmd );

	return drive;
}

static int32 SendCMD( SDL_CD *cdrom, const uint8 *cmd, int cmdlen )
{
	struct amigacd *entry = cddrives[cdrom->id];
	if( !entry ) return -1;
	
	entry->scsicmd.scsi_Data        = entry->inbuf;
	entry->scsicmd.scsi_Length      = SCSI_INBUF_SIZE;
	entry->scsicmd.scsi_SenseActual = 0;
	entry->scsicmd.scsi_SenseData   = (uint8 *)entry->sensebuf;
	entry->scsicmd.scsi_SenseLength = SCSI_SENSE_SIZE;
	entry->scsicmd.scsi_CmdActual   = 0;
	entry->scsicmd.scsi_Command     = (uint8 *)cmd;
	entry->scsicmd.scsi_CmdLength   = cmdlen;
	entry->scsicmd.scsi_Flags       = SCSIF_READ | SCSIF_AUTOSENSE;
	
	IExec->DoIO( (struct IORequest *)entry->req );
	
	return entry->scsicmd.scsi_Status ? -1 : 0;
}

static int SDL_SYS_CDGetTOC(SDL_CD *cdrom)
{
	static const uint8 cmd[] = { 0x43, 0, 0, 0, 0, 0, 0, TOC_SIZE>>8, TOC_SIZE&0xff, 0 };
	int trackoffs;
	BOOL ok;
	
	ok = FALSE;
	if( SendCMD( cdrom, cmd, sizeof( cmd ) ) == 0 )
	{
		uint8 *b;
		struct amigacd *entry = cddrives[cdrom->id];
		int32 i;
		
		b = (uint8 *)entry->inbuf;
		
//		IExec->DebugPrintF( "First: %ld, Last: %ld\n",
//		  (int32)b[2], (int32)b[3] );

		cdrom->numtracks = (b[3] - b[2])+1;
//		IExec->DebugPrintF( "Tracks: %ld\n", (int32)cdrom->numtracks );
		
		if( cdrom->numtracks > SDL_MAX_TRACKS )
			cdrom->numtracks = SDL_MAX_TRACKS;

		ok = TRUE;
		for( i=0; i<=cdrom->numtracks; i++ )
		{
			cdrom->track[i].id = i+1;
			
			trackoffs = i*8+4;
			if( b[trackoffs+1]&4 )
				cdrom->track[i].type = SDL_AUDIO_TRACK;
			else
				cdrom->track[i].type = SDL_DATA_TRACK;

			cdrom->track[i].offset = (b[trackoffs+4]<<24)|(b[trackoffs+5]<<16)|(b[trackoffs+6]<<8)|b[trackoffs+7];
			cdrom->track[i].length = 0;
			if( i > 0 )
			{
				cdrom->track[i-1].length = cdrom->track[i].offset - cdrom->track[i-1].offset;
//				IExec->DebugPrintF( "%ld: Offset: %ld, Tracklen: %ld\n", (int32)i, (int32)cdrom->track[i-1].offset, (int32)cdrom->track[i-1].length );
			}
		}
	}
	
	return ok ? 0 : -1;
}

static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position)
{
	static const uint8 cmd[] = { 0x42, 0, 0x40, 0x01, 0, 0, 0, SCSI_INBUF_SIZE>>8, SCSI_INBUF_SIZE&0xff, 0 };
	static const uint8 rdycmd[] = { 0x00, 0, 0, 0, 0, 0 };
	CDstatus status;

	status = CD_ERROR;

	if( SendCMD( cdrom, rdycmd, sizeof( rdycmd ) ) != 0 )
	  return CD_TRAYEMPTY;
	
	if( SendCMD( cdrom, cmd, sizeof( cmd ) ) == 0 )
	{
		struct amigacd *entry = cddrives[cdrom->id];
		uint8 *buf;

		buf = (uint8 *)entry->inbuf;
		switch( buf[1] )
		{
			case 0x11: status = CD_PLAYING; break;
			case 0x12: status = CD_PAUSED;  break;
			default:   status = CD_STOPPED; break;
		}
	}

	return status;
}

static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length)
{
	uint8 cmd[] = { 0xa5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//	IExec->DebugPrintF( "PlayCD: %ld, %ld\n", (int32)start, (int32)length );

	cmd[2] = start>>24;
	cmd[3] = start>>16;
	cmd[4] = start>>8;
	cmd[5] = start;
	cmd[6] = length>>24;
	cmd[7] = length>>16;
	cmd[8] = length>>8;
	cmd[9] = length;
	
	SendCMD( cdrom, cmd, sizeof( cmd ) );
	return 0;
}

static int SDL_SYS_CDPause(SDL_CD *cdrom)
{
	static const uint8 cmd[] = { 0x4b, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	SendCMD( cdrom, cmd, sizeof( cmd ) );
	return 0;
}

static int SDL_SYS_CDResume(SDL_CD *cdrom)
{
	static const uint8 cmd[] = { 0x4b, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
	SendCMD( cdrom, cmd, sizeof( cmd ) );
	return 0;
}

static int SDL_SYS_CDStop(SDL_CD *cdrom)
{
	static const uint8 cmd[] = { 0x4e, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
	SendCMD( cdrom, cmd, sizeof( cmd ) );
	return 0;
}

static int SDL_SYS_CDEject(SDL_CD *cdrom)
{
	static const uint8 cmd[] = { 0x1b, 0, 0, 0, 0x02, 0 };
	SendCMD( cdrom, cmd, sizeof( cmd ) );
	return 0;
}

static void SDL_SYS_CDClose(SDL_CD *cdrom)
{
	struct amigacd *entry = cddrives[cdrom->id];

	if( !entry ) return;

	if( entry->req )
	{
		IExec->CloseDevice( (struct IORequest *)entry->req );
		IExec->FreeSysObject( ASOT_IOREQUEST, entry->req );
	}

	if( entry->port )
		IExec->FreeSysObject( ASOT_PORT, entry->port );

	if( entry->inbuf )
		IExec->FreeVec( entry->inbuf );

	if( entry->sensebuf )
		IExec->FreeVec( entry->sensebuf );

	entry->req = NULL;
	entry->port = NULL;
	entry->inbuf = NULL;
	entry->sensebuf = NULL;
}

