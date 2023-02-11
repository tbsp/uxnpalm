#include "uxncli.h"
#include <PalmOS.h>

#define	uxncliAppID 'UxnV'
#define	uxncliDBType 'ROMU'

static void *GetObjectPtr(UInt16 objectID)
{
    FormType *form = FrmGetActiveForm();
    return (FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID)));
}

static void InitializeROMList()
{
  UInt16            numROMs;
  static char     **nameROMs;
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

static Boolean AppHandleEvent(EventPtr event)
{
  FormPtr pfrm;
  Int32 formId;
  Boolean handled;

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
