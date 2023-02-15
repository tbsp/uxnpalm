#include "uxncli.h"
#include <PalmOS.h>
#include "uxn.h"
#include "devices/system.h"

#define	uxncliAppID 'UxnV'
#define	uxncliDBType 'ROMU'

//char      *consoleOutput;
//char      *outputPtr;

#define SUPPORT 0x1c03 /* devices mask */


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
console_deo(Uint8 *d, Uint8 port)
{
  if(port==0x8) {
    FieldType *fld = GetObjectPtr(fieldID_desc);

    void *ptr = FldGetTextPtr(fld);

    UInt8 len = StrLen(ptr);
    char *newStr = MemPtrNew(len + 2);
    StrCopy(newStr, ptr);
    newStr[len] = d[port];
    newStr[len + 1] = '\0';

    FldSetTextPtr(fld, newStr);
    MemPtrFree(ptr);

	  FldRecalculateField(fld, true);
    FldDrawField(fld);
  }
}

static Uint8
emu_dei(Uxn *u, Uint8 addr)
{
	Uint8 p = addr & 0x0f, d = addr & 0xf0;
	switch(d) {
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
	Uint16 mask = 0x1 << (d >> 4);
	u->dev[addr] = v;
	switch(d) {
	//case 0x00: system_deo(u, &u->dev[d], p); break;
	case 0x10: console_deo(&u->dev[d], p); break;
	//case 0xa0: file_deo(0, u->ram, &u->dev[d], p); break;
	//case 0xb0: file_deo(1, u->ram, &u->dev[d], p); break;
	}
	if(p == 0x01 && !(SUPPORT & mask))
		ErrDisplay("Warning: Incompatible emulation, device ?");
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
          fld = GetObjectPtr(fieldID_desc);
          consoleOutput = MemPtrNew(1);
	        FldSetTextPtr(fld, consoleOutput);

          MemSemaphoreReserve(true);
          //ErrDisplay(romName);

          Uxn u;

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

          MemSemaphoreRelease(true);

          break;
        default:
          break;
      }
      break;
    case frmOpenEvent:
      pfrm = FrmGetActiveForm();
      FrmDrawForm(pfrm);
      break;
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
