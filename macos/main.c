#include "main.h"

#include "error.h"
#include "project.h"
#include "resources.h"

#ifndef __MWERKS__
// Link error if defined with CodeWarrior.
QDGlobals qd;
#endif

void QuitApp(void)
{
	ExitToShell();
}

static void HandleClose(WindowRef window)
{
	ProjectHandle project;

	if (window == NULL) {
		return;
	}
	switch (GetWindowKind(window)) {
	case kApplicationWindowKind:
		project = (ProjectHandle)GetWRefCon(window);
		ASSERT(project != NULL);
		ProjectCommand(window, project, rMENU_File, iFile_Close);
		break;
	}
}

static void AdjustMenus(void)
{
	WindowRef window;
	MenuHandle fileMenu;
	ProjectHandle project;
	Boolean hasDocument = FALSE;

	fileMenu = GetMenuHandle(rMENU_File);
	window = FrontWindow();
	if (window != NULL) {
		switch (GetWindowKind(window)) {
		case kApplicationWindowKind:
			project = (ProjectHandle)GetWRefCon(window);
			ASSERT(project != NULL);
			hasDocument = TRUE;
			ProjectAdjustMenus(project, fileMenu);
			break;
		}
	}

	if (fileMenu != NULL) {
		EnableItem(fileMenu, iFile_New);
		DisableItem(fileMenu, iFile_Open);
		if (!hasDocument) {
			DisableItem(fileMenu, iFile_Close);
			DisableItem(fileMenu, iFile_Save);
			DisableItem(fileMenu, iFile_SaveAs);
			DisableItem(fileMenu, iFile_Revert);
		}
		EnableItem(fileMenu, iFile_Quit);
	}
}

static void HandleMenuCommand(long command)
{
	int menuID = command >> 16;
	int item = command & 0xffff;
	WindowRef window;
	ProjectHandle project;

	switch (menuID) {
	case rMENU_Apple:
		switch (item) {
		case iApple_Info:
			return;
		default: {
			Str255 itemName;
			GetMenuItemText(GetMenuHandle(rMENU_Apple), item, itemName);
			OpenDeskAcc(itemName);
		}
			return;
		}
		return;
	case rMENU_File:
		switch (item) {
		case iFile_New:
			ProjectNew();
			return;
		case iFile_Quit:
			QuitApp();
			return;
		}
		break;
	case rMENU_Edit:
		break;
	}

	window = FrontWindow();
	if (window != NULL) {
		switch (GetWindowKind(window)) {
		case kApplicationWindowKind:
			project = (ProjectHandle)GetWRefCon(window);
			ASSERT(project != NULL);
			ProjectCommand(window, project, menuID, item);
			return;
		}
	}

	SysBeep(1);
}

// Handle a mouse down event.
static void HandleMouseDown(EventRecord *event)
{
	short clickPart;
	WindowPtr window;
	ProjectHandle project;

	clickPart = FindWindow(event->where, &window);
	switch (clickPart) {
		// case inDesk:

	case inMenuBar:
		AdjustMenus();
		HandleMenuCommand(MenuSelect(event->where));
		HiliteMenu(0);
		break;

	case inSysWindow:
		SystemClick(event, window);
		break;

	case inContent:
		if (window != FrontWindow()) {
			SelectWindow(window);
		} else {
			switch (GetWindowKind(window)) {
			case kApplicationWindowKind:
				project = (ProjectHandle)GetWRefCon(window);
				ASSERT(project != NULL);
				ProjectMouseDown(window, project, event);
				break;
			}
		}
		break;

	case inDrag:
		if ((event->modifiers & cmdKey) != 0 && window != FrontWindow()) {
			SelectWindow(window);
		}
		DragWindow(window, event->where, &(*GetGrayRgn())->rgnBBox);
		break;

		// case inGrow:

	case inGoAway:
		if (TrackGoAway(window, event->where)) {
			HandleClose(window);
		}
		break;

		// case inZoomIn:
		// case inZoomOut:
		// case inCollapseBox:
		// case inProxyIcon:
	}
}

static void HandleKeyDown(EventRecord *event)
{
	long command;

	if ((event->modifiers & cmdKey) != 0) {
		AdjustMenus();
		command = MenuKey(event->message & charCodeMask);
		HandleMenuCommand(command);
		HiliteMenu(0);
	}
}

static void HandleActivate(EventRecord *event)
{
	ProjectHandle project;
	WindowRef window;
	GrafPtr savedPort;
	int isActive;

	window = (WindowRef)event->message;
	GetPort(&savedPort);
	SetPort(window);
	isActive = (event->modifiers & activeFlag) != 0;
	switch (GetWindowKind(window)) {
	case kApplicationWindowKind:
		project = (ProjectHandle)GetWRefCon(window);
		ASSERT(project != NULL);
		ProjectActivate(window, project, isActive);
		break;
	}
	SetPort(savedPort);
}

static void HandleUpdate(EventRecord *event)
{
	ProjectHandle project;
	WindowPtr window;
	GrafPtr savedPort;

	window = (WindowPtr)event->message;
	GetPort(&savedPort);
	SetPort(window);
	BeginUpdate(window);
	switch (GetWindowKind(window)) {
	case kApplicationWindowKind:
		project = (ProjectHandle)GetWRefCon(window);
		ASSERT(project != NULL);
		ProjectUpdate(window, project);
		break;
	}
	EndUpdate(window);
	SetPort(savedPort);
}

void main(void)
{
	MaxApplZone();
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NULL);
	InitCursor();

	// Set up menu bar.
	{
		Handle menuBar;
		menuBar = GetNewMBar(rMBAR_Main);
		if (menuBar == NULL) {
			EXIT_ASSERT(NULL);
		}
		SetMenuBar(menuBar);
		DisposeHandle(menuBar);
		AppendResMenu(GetMenuHandle(rMENU_Apple), 'DRVR');
		DrawMenuBar();
	}

	for (;;) {
		EventRecord event;
		WaitNextEvent(everyEvent, &event, 1, NULL);
		switch (event.what) {
		case mouseDown:
			HandleMouseDown(&event);
			break;

			// case mouseUp:

		case keyDown:
			HandleKeyDown(&event);
			break;

			// case autoKey:

		case activateEvt:
			HandleActivate(&event);
			break;

		case updateEvt:
			HandleUpdate(&event);
			break;

			// case diskEvt:
			// osEvt:
			// case kHighLevelEvent:
		}
	}
}
