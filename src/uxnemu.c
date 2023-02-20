#include <PalmOS.h>
#include "ui_IDs.h"
#include "uxn.h"
#include "uxnemu.h"
#include "devices/system.h"
#include "devices/screen.h"

#define	uxncliAppID 'UxnV'
#define	uxncliDBType 'ROMU'

#define SUPPORT 0x1c03 /* devices mask */

/*
Copyright (c) 2021-2023 Devine Lu Linvega, Andrew Alderwick
Copyright (c) 2023 Dave VanEe

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define WIDTH 160
#define HEIGHT 160

void* alloc(UInt32 qtty, UInt8 sz) {
    MemPtr p = MemPtrNew(sz*qtty);
    MemSet(p, sz*qtty, 0);
    return p;
}

static void *GetObjectPtr(UInt16 objectID)
{
    FormType *form = FrmGetActiveForm();
    return (FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID)));
}

static void InitializeROMList()
{
  UInt16            numROMs;
  char            **nameROMs;
  UInt16            cardNo;
  UInt32            dbType;
  DmSearchStateType searchState;
  LocalID           dbID;
  char              name[33];
  Boolean           first;

  /* First count number of ROMs */
  numROMs = 0;
  first = true;
  while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, uxncliAppID, false, &cardNo, &dbID))
  {
    first = false;
    DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
    if (dbType == uxncliDBType)
      numROMs++;
  }
  nameROMs = MemPtrNew(numROMs * sizeof(Char *));

  numROMs = 0;
  first = true;
  while (!DmGetNextDatabaseByTypeCreator(first, &searchState, NULL, uxncliAppID, false, &cardNo, &dbID))
  {
      first = false;
      DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dbType, NULL);
      if (dbType == uxncliDBType)
      {
        nameROMs[numROMs] = MemPtrNew(32);
        StrCopy(nameROMs[numROMs], name);
        numROMs++;

      }
  }
  LstSetListChoices(GetObjectPtr(listID_roms), nameROMs, numROMs);
 }

static void
redraw(void)
{
  struct UxnScreen *uxn_screen;
  uxn_screen = (struct UxnScreen*)globalsSlotVal(GLOBALS_SLOT_SCREEN_DEVICE);
	screen_redraw(uxn_screen, uxn_screen->pixels);
  // Write pixels to window
  // TODO: Streamline all this, which is just an ugly hack right now
  for(Uint16 y=0; y<uxn_screen->height; y++)
    for(Uint16 x=0; x<uxn_screen->width; x++) {
      Uint32 i = x + y * uxn_screen->width;
      WinSetForeColor(uxn_screen->pixels[i]);
      WinDrawPixel(x, y);
    }
}

static Uint8
emu_dei(Uxn *u, Uint8 addr)
{
	Uint8 p = addr & 0x0f, d = addr & 0xf0;
	switch(d) {
	case 0x20: return screen_dei(&u->dev[d], p);
	//case 0x30: return audio_dei(0, &u->dev[d], p);
	//case 0x40: return audio_dei(1, &u->dev[d], p);
	//case 0x50: return audio_dei(2, &u->dev[d], p);
	//case 0x60: return audio_dei(3, &u->dev[d], p);
	//case 0xa0: return file_dei(0, &u->dev[d], p);
	//case 0xb0: return file_dei(1, &u->dev[d], p);
	//case 0xc0: return datetime_dei(&u->dev[d], p);
	}
	return u->dev[addr];
}

static void
emu_deo(Uxn *u, Uint8 addr, Uint8 v)
{
	Uint8 p = addr & 0x0f, d = addr & 0xf0;
  struct UxnScreen *uxn_screen;
	u->dev[addr] = v;
	switch(d) {
	case 0x00:
		system_deo(u, &u->dev[d], p);
		if(p > 0x7 && p < 0xe)
	    uxn_screen = (struct UxnScreen*)globalsSlotVal(GLOBALS_SLOT_SCREEN_DEVICE);
			screen_palette(uxn_screen, &u->dev[0x8]);
		break;
	//case 0x10: console_deo(&u->dev[d], p); break;
	case 0x20: screen_deo(u->ram, &u->dev[d], p); break;
	//case 0x30: audio_deo(0, &u->dev[d], p, u); break;
	//case 0x40: audio_deo(1, &u->dev[d], p, u); break;
	//case 0x50: audio_deo(2, &u->dev[d], p, u); break;
	//case 0x60: audio_deo(3, &u->dev[d], p, u); break;
	//case 0xa0: file_deo(0, u->ram, &u->dev[d], p); break;
	//case 0xb0: file_deo(1, u->ram, &u->dev[d], p); break;
	}
}

static Boolean AppHandleEvent(EventPtr event)
{
  UInt16 i;
  FormPtr pfrm;
  Int32 formId;
  Boolean handled;
  char romName[33];

  FieldType *fld;
  char *consoleOutput;

  switch(event->eType) {
    case frmLoadEvent:
      formId = event->data.frmLoad.formID;
      pfrm = FrmInitForm(formId);
      FrmSetActiveForm(pfrm);
      switch(formId)
      {
        case formID_load:
          InitializeROMList();
          break;
        case formID_run:

          MemSemaphoreReserve(true);

          Uxn u;

          // Initialize and store the global screen pointer
	        struct UxnScreen *uxn_screen;
          uxn_screen = MemPtrNew(sizeof(UxnScreen));
	        //uxn_screen = (struct UxnScreen*)globalsSlotVal(GLOBALS_SLOT_SCREEN_DEVICE);
          *globalsSlotPtr(GLOBALS_SLOT_SCREEN_DEVICE) = uxn_screen;
          screen_resize(uxn_screen, WIDTH, HEIGHT);

          // This boot/load/eval is 'start' in ref uxnemu
          // TODO: Figure out why alloc fails with the full 64KB memory space
          if(!uxn_boot(&u, (Uint8 *)alloc(0xf000, sizeof(Uint8)), 
                           (Uint8 *)alloc(0x200, sizeof(Uint8)),
                           (Uint8 *)alloc(0x100, sizeof(Uint8)),
                           emu_dei, emu_deo))
            return 1;
          if(!system_load(&u, romName))
            return 1;
          if(!uxn_eval(&u, PAGE_PROGRAM))
            return 1;

            // TOOD: Change to proper 'run' loop with redraw
          if(uxn_screen->fg.changed || uxn_screen->bg.changed)
              redraw();

          MemSemaphoreRelease(true);

          break;
        default:
          break;
      }
      break;
    case frmOpenEvent:
      formId = event->data.frmLoad.formID;
      switch(formId)
      {
        case formID_load:
        pfrm = FrmGetActiveForm();
        FrmDrawForm(pfrm);
        break;
      }
    case menuEvent:
      break;
    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID)
      {
        case buttonID_run:
          i = LstGetSelection(GetObjectPtr(listID_roms));
          StrCopy(romName, LstGetSelectionText(GetObjectPtr(listID_roms), i));

          FrmGotoForm(formID_run);

          break;
      }
      break;
    case appStopEvent:
      break;
    default:
      if(FrmGetActiveForm())
        FrmHandleEvent(FrmGetActiveForm(), event);
  }
  handled = true;
  return handled;
}

static void AppStart()
{
	FrmGotoForm(formID_load);
}

static void AppEventLoop(void)
{
  EventType event;
  UInt16 error;

  do {
    EvtGetEvent(&event, evtWaitForever);

    if(SysHandleEvent(&event))
      continue;
    if(MenuHandleEvent((void *)0, &event, &error))
      continue;
    if(AppHandleEvent(&event))
      continue;

  } while (event.eType != appStopEvent);
}

static void AppStop()
{
  FrmCloseAllForms();

  struct UxnScreen *uxn_screen;
  uxn_screen = (struct UxnScreen*)globalsSlotVal(GLOBALS_SLOT_SCREEN_DEVICE);
  MemPtrFree(uxn_screen->bg.pixels);
  MemPtrFree(uxn_screen->fg.pixels);
  MemPtrFree(uxn_screen->pixels);
  MemPtrFree(uxn_screen);
}

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {

	if (sysAppLaunchCmdNormalLaunch == cmd) {
		AppStart();
		AppEventLoop();
    AppStop();
	}

	return 0;
}

UInt32 __attribute__((section(".vectors"))) __Startup__(void)
{
	SysAppInfoPtr appInfoP;
	void *prevGlobalsP;
	void *globalsP;
	UInt32 ret;
	
	SysAppStartup(&appInfoP, &prevGlobalsP, &globalsP);
	ret = PilotMain(appInfoP->cmd, appInfoP->cmdPBP, appInfoP->launchFlags);
	SysAppExit(appInfoP, prevGlobalsP, globalsP);
	
	return ret;
}
